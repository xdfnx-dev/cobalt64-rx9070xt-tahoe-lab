// Fixed DMUB version query and read-only firmware window access.
// No clock, stream, timing, PHY, AUX, GPU reset or power writes are exposed.
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOLib.h>
#include <libkern/OSAtomic.h>
#include <kern/task.h>
#include <string.h>
#include "Navi48Bringup.hpp"
#include "dcn41_regs.h"
#include "ABI.h"
#include "fw_query.hpp"

bool navi48_vram_read_mm(uint64_t off, uint32_t *dst, uint32_t dwords);

class Navi48DPControl : public IOService {
    OSDeclareDefaultStructors(Navi48DPControl)
    Navi48Bringup *gpu=nullptr;
    amdgpu::DeviceContext *dev=nullptr;
    IOLock *lock=nullptr;
    bool queried=false, queryFailed=false;
    n48dp_report cached{};
    bool read(uint32_t reg,uint32_t&value) const {
        if(!dev || !dev->rmmio || (uint64_t)reg*4+4>dev->rmmioSize)return false;
        value=amdgpu::RREG32(*dev,reg);return value!=0xffffffffu;
    }
    bool window(unsigned inst,n48dp_window&w) const {
        if(inst>1)return false;
        if(!read(0x34c0+0x1a5+inst,w.base) || !read(0x34c0+0x1ad+inst,w.top) ||
           !read(0x34c0+0x1b5+inst*2,w.offset) || !read(0x34c0+0x1b6+inst*2,w.offset_high))return false;
        if(!(w.top&0x80000000u))return false;
        uint32_t base=w.base&0x1fffffffu,top=w.top&0x1fffffffu;
        if(top<base)return false;
        w.size=(uint64_t)top-base+1;
        uint64_t mc=((uint64_t)w.offset_high<<32)|w.offset;
        if(mc<dev->vramMcBase || w.size>4u*1024u*1024u)return false;
        w.vram_offset=mc-dev->vramMcBase;
        return w.vram_offset<dev->vramSizeBytes && w.size<=dev->vramSizeBytes-w.vram_offset;
    }
    bool census(n48dp_report&r) const {
        r.abi=N48DP_ABI;
        if(!read(0x34c0+DCN41_DMCUB_GPINT_DATAIN1,r.gpint_before) || !read(0x34c0+DCN41_DMCUB_SCRATCH0,r.scratch0) ||
           !read(0x5827,r.pixel_format) || !read(0x5828,r.msa_colorimetry) ||
           !read(0x582a,r.stream) || !read(0x5826,r.link) ||
           !read(0x34c0+DCN41_OTG_OTG_STATUS_FRAME_COUNT(0),r.frame_before))return false;
        r.gpint_after=r.gpint_before;
        for(unsigned i=0;i<2;i++)if(!window(i,r.window[i]))r.window[i].size=0;
        return true;
    }
    struct VersionEnv {
        Navi48DPControl*self;
        bool read(uint32_t reg,uint32_t&v){return self->read(reg,v);}
        void versionRequest(){
            amdgpu::WREG32(*self->dev,0x34c0+DCN41_DMCUB_GPINT_DATAIN1,0x10010000u);
            amdgpu::storeFence();
        }
        void pause10us(){IODelay(10);}
    };
public:
    bool start(IOService*p) override {
        gpu=OSDynamicCast(Navi48Bringup,p);
        if(!gpu || !IOService::start(p))return false;
        auto*c=gpu->bringupContext();
        dev=c?c->dev:nullptr;
        if(!dev || !dev->rmmio || dev->vramSizeBytes!=(16304ull<<20)){
            IOLog("Navi48DPControl: refused: VRAM size mismatch (expected live RCC_CONFIG_MEMSIZE 16304 MiB)\n");return false;
        }
        auto v=dev->ip.getVersion(amdgpu::IPBlock::GC);
        if(v.major!=12 || v.minor!=0 || v.rev!=1){IOLog("Navi48DPControl: refused: GC version mismatch\n");return false;}
        uint32_t dmuByte=0;
        if(!gpu->ipDisc().regByteOffset(IpDiscovery::HwDmu,0,2,0,dmuByte) || dmuByte!=0x34c0u*4u){IOLog("Navi48DPControl: refused: DMU segment mismatch\n");return false;}
        auto*result=OSDynamicCast(OSNumber,p->getProperty("Navi48,StageResult"));
        if(!result || result->unsigned32BitValue()!=0){IOLog("Navi48DPControl: refused: bringup stage not successful\n");return false;}
        lock=IOLockAlloc();if(!lock)return false;
        setProperty("DP120PhysicalModeEnabled",false);
        setProperty("DP120Commands","census, fixed GET_FW_VERSION, bounded code-window read");
        registerService();IOLog("Navi48DPControl: fixed firmware-query service ready (0.0.2)\n");return true;
    }
    void free() override {
        if(lock){IOLockFree(lock);lock=nullptr;}
        IOService::free();
    }
    IOReturn newUserClient(task_t,void*,UInt32,OSDictionary*,IOUserClient**) override;
    IOReturn report(bool version,n48dp_report&r) {
        IOLockLock(lock);
        memset(&r,0,sizeof r);
        if(version && queried){r=cached;IOLockUnlock(lock);return queryFailed?kIOReturnTimeout:kIOReturnSuccess;}
        if(!census(r)){IOLockUnlock(lock);return kIOReturnNoDevice;}
        r.op=version?N48DP_SEL_FW_VERSION:N48DP_SEL_CENSUS;
        if(version){
            VersionEnv env{this};auto q=Dp120::queryFirmwareVersion(env);
            r.status=q.status;r.fw_version=q.version;r.gpint_before=q.before;r.gpint_after=q.after;r.polls=q.polls;
            if(!q.sent){IOLockUnlock(lock);return q.status==Dp120::FwBusy?kIOReturnBusy:kIOReturnNoDevice;}
            queried=true;queryFailed=q.status!=Dp120::FwOk;
            if(!read(0x34c0+DCN41_OTG_OTG_STATUS_FRAME_COUNT(0),r.frame_after)){r.status=2;queryFailed=true;}
            cached=r;
            setProperty("DP120GPINTVersion",r.fw_version,32);
            setProperty("DP120GPINTStatus",r.status,32);
        }
        IOLockUnlock(lock);
        return r.status?kIOReturnTimeout:kIOReturnSuccess;
    }
    IOReturn codeRead(unsigned inst,uint32_t off,uint32_t*out) {
        IOLockLock(lock);n48dp_window w{};
        bool ok=inst<=1 && !(off&3u) && window(inst,w) &&
                off<=w.size && 256u<=w.size-off &&
                navi48_vram_read_mm(w.vram_offset+off,out,64);
        IOLockUnlock(lock);return ok?kIOReturnSuccess:kIOReturnBadArgument;
    }
};

class Navi48DPClient : public IOUserClient {
    OSDeclareDefaultStructors(Navi48DPClient)
    Navi48DPControl*owner=nullptr;
public:
    bool initWithTask(task_t task,void*security,UInt32 type,OSDictionary*props) override {
        return type==N48DP_CLIENT_TYPE &&
               clientHasPrivilege(security,kIOClientPrivilegeAdministrator)==kIOReturnSuccess &&
               IOUserClient::initWithTask(task,security,type,props);
    }
    bool start(IOService*p) override {
        owner=OSDynamicCast(Navi48DPControl,p);return owner && IOUserClient::start(p);
    }
    IOReturn clientClose() override {terminate();return kIOReturnSuccess;}
    IOReturn externalMethod(uint32_t sel,IOExternalMethodArguments*a,
            IOExternalMethodDispatch*d=nullptr,OSObject*t=nullptr,void*r=nullptr) override {
        (void)d;(void)t;(void)r;
        if(!owner || !a || a->structureInputSize || a->structureOutputDescriptor ||
           a->structureInputDescriptor || a->scalarOutputCount)return kIOReturnBadArgument;
        if((sel==N48DP_SEL_CENSUS || sel==N48DP_SEL_FW_VERSION) &&
           a->scalarInputCount==0 && a->structureOutput && a->structureOutputSize==sizeof(n48dp_report)){
            n48dp_report out{};IOReturn rc=owner->report(sel==N48DP_SEL_FW_VERSION,out);
            memcpy(a->structureOutput,&out,sizeof out);a->structureOutputSize=sizeof out;return rc;
        }
        if(sel==N48DP_SEL_CODE_READ && a->scalarInputCount==2 &&
           a->scalarInput[0]<=1 && a->scalarInput[1]<=0xffffffffu &&
           a->structureOutput && a->structureOutputSize==256){
            uint32_t out[64]={};IOReturn rc=owner->codeRead((unsigned)a->scalarInput[0],(uint32_t)a->scalarInput[1],out);
            if(rc==kIOReturnSuccess){memcpy(a->structureOutput,out,256);a->structureOutputSize=256;}
            return rc;
        }
        return kIOReturnUnsupported;
    }
};
OSDefineMetaClassAndStructors(Navi48DPControl,IOService)
OSDefineMetaClassAndStructors(Navi48DPClient,IOUserClient)
IOReturn Navi48DPControl::newUserClient(task_t task,void*security,UInt32 type,OSDictionary*props,IOUserClient**out) {
    if(!out || type!=N48DP_CLIENT_TYPE)return kIOReturnBadArgument;*out=nullptr;
    auto*c=new Navi48DPClient;
    if(!c)return kIOReturnNoMemory;
    if(!c->initWithTask(task,security,type,props)){c->release();return kIOReturnNotPermitted;}
    if(!c->attach(this)){c->release();return kIOReturnError;}
    if(!c->start(this)){c->detach(this);c->release();return kIOReturnError;}
    *out=c;return kIOReturnSuccess;
}
