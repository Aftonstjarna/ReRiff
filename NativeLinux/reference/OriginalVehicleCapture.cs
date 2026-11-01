// Original routines execute unchanged. This fixture provides only their object
// graph and boundary inputs. No replacement control/physics formulas run here.
using System;
using System.IO;
using System.Runtime.InteropServices;

unsafe partial class OriginalSongCapture
{
    static void VF(IntPtr p,int offset,float value) { *(float*)((byte*)p+offset)=value; }
    static float VF(IntPtr p,int offset) { return *(float*)((byte*)p+offset); }
    static void VP(IntPtr p,int offset,IntPtr value) { *(IntPtr*)((byte*)p+offset)=value; }
    static void VI(IntPtr p,int offset,int value) { *(int*)((byte*)p+offset)=value; }
    static void VB(IntPtr p,int offset,bool value) { ((byte*)p)[offset]=(byte)(value?1:0); }
    static void V3(IntPtr p,int offset,float x,float y,float z) { VF(p,offset,x);VF(p,offset+4,y);VF(p,offset+8,z); }
    static void Row(TextWriter w,string name,params float[] values) {
        w.Write(name);foreach(float value in values) { w.Write('\t');w.Write(F(value)); }w.WriteLine();
    }
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)] delegate IntPtr GetDeviceFn(IntPtr self,uint id);
    static GetDeviceFn getDevice=delegate(IntPtr self,uint id) { return IntPtr.Zero; };
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)] delegate void ApplyAtFn(IntPtr self,IntPtr impulse,IntPtr point);
    static float[] capturedImpulse=new float[3];
    static int impulseCalls;
    static ApplyAtFn forwardWheelForce;
    static float[] wheelForce=new float[3],wheelPoint=new float[3];
    static int wheelForceCalls;
    static float[,] allWheelForces=new float[4,3],allWheelPoints=new float[4,3];
    static ApplyAtFn captureWheelForce=delegate(IntPtr self,IntPtr force,IntPtr point) {
        for(int i=0;i<3;i++){wheelForce[i]=VF(force,i*4);wheelPoint[i]=VF(point,i*4);}
        if(wheelForceCalls<4)for(int i=0;i<3;i++){allWheelForces[wheelForceCalls,i]=wheelForce[i];allWheelPoints[wheelForceCalls,i]=wheelPoint[i];}
        ++wheelForceCalls;forwardWheelForce(self,force,point);
    };
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)] delegate void ApplyTorqueFn(IntPtr self,IntPtr torque);
    static float[] capturedTorque=new float[3];
    static int torqueCalls;
    static ApplyTorqueFn applyTorque=delegate(IntPtr self,IntPtr torque) {
        for(int i=0;i<3;i++)capturedTorque[i]=VF(torque,i*4);
        ++torqueCalls;
    };
    static ApplyAtFn applyAt=delegate(IntPtr self,IntPtr impulse,IntPtr point) {
        for(int i=0;i<3;i++)capturedImpulse[i]=VF(impulse,i*4);
        if(VF(point,0)!=0 || VF(point,4)!=0 || VF(point,8)!=0)throw new InvalidOperationException("Drive impulse was not at the center");
        ++impulseCalls;
    };
    static void CaptureVehicleBehavior(string input,string output)
    {
        Console.WriteLine("CAPTURE=original vehicle behavior");
        IntPtr player=Alloc(2048),vehicle=Alloc(2048),parameters=Alloc(1024),body=Alloc(1024),
            holder=Alloc(16),component=Alloc(512),tracker=Alloc(128),node0=Alloc(244),node1=Alloc(244),
            race=Alloc(128),pace=Alloc(128),gearbox=Alloc(256),wheel=Alloc(128),wheelPointers=Alloc(16),
            pedals=Alloc(8),spring=Alloc(16),inputManager=Alloc(192),deviceManager=Alloc(4),deviceTable=Alloc(8);
        VP(player,72,holder);VP(holder,0,vehicle);VP(holder,4,component);VP(holder,8,player);
        VP(player,4,tracker);VP(vehicle,328,holder);VP(vehicle,332,parameters);VP(vehicle,824,gearbox);
        VP(tracker,72,node0);VP(tracker,80,node1);VF(node0,208,1);VF(node0,212,1);VF(node1,208,1);VF(node1,212,1);
        // Native getVel/setVel disassembly: body pointer at +0x78, velocity +0x130.
        // Native getAngVel: +0x140. These are scratch data, not a Bullet simulation.
        VP(vehicle,120,body);V3(vehicle,704,0,-1,0);V3(vehicle,32,0,0,1);
        VP(race,4,pace);VI(pace,4,1);VP(vehicle,420,wheelPointers);VP(vehicle,424,IntPtr.Add(wheelPointers,16));
        for(int i=0;i<4;i++)VP(wheelPointers,i*4,wheel);
        VI(parameters,312,4);VF(parameters,332,80);
        VP(deviceTable,4,Marshal.GetFunctionPointerForDelegate(getDevice));VP(deviceManager,0,deviceTable);
        VB(player,292,true);VB(player,858,true);
        string raceName="?ms_pInstance@gaRaceManager@@0PAV1@A", inputName="?ms_pInstance@gaInputManager@@0PAV1@A",
            deviceName="?ms_instance@?$elSingleton@VelInputManager@@@@0PAVelInputManager@@A", dtName="?dt@gaTimer@@2MA";
        IntPtr oldRace=Global(raceName),oldInput=Global(inputName),oldDevice=Global(deviceName);
        object oldDt=Field(dtName).GetValue(null);
        SetGlobal(raceName,race);SetGlobal(inputName,inputManager);SetGlobal(deviceName,deviceManager);
        try {
            using(var w=new StreamWriter(Path.Combine(output,"original-behavior.tsv"))) {
                w.WriteLine("# Original Game.exe methods; x86 Microsoft CLR; no patched method bodies");
                foreach(float k in new float[]{0,5,10,15,30,100,155,190,200,205,299,300,450})
                    Row(w,"friction-table",k,(float)Call("gaSpring.getMatchingFriction",k,false));
                foreach(float k in new float[]{30,200,450}) {
                    Call("gaSpring.{ctor}",spring,k);
                    for(int step=0;step<180;step++) {
                        float dt=step%3==0?1f/120:step%3==1?1f/60:1f/30;
                        float target=step<60?20:step<120?-10:0, beforeVelocity=VF(spring,8),beforePosition=VF(spring,12);
                        Call("gaSpring.tick",spring,target,dt);
                        Row(w,"spring",k,VF(spring,4),dt,target,beforeVelocity,beforePosition,VF(spring,8),VF(spring,12));
                    }
                }
                foreach(bool active in new bool[]{false,true}) foreach(bool ramp in new bool[]{false,true})
                foreach(float target in new float[]{0,5,30,80}) foreach(float speed in new float[]{-5,0,4.9f,5,5.1f,30,90}) {
                    VB(race,33,active);VB(vehicle,1319,ramp);VF(player,776,target);VF(vehicle,360,speed);
                    Call("gaPlayerLocal.calcAutoAccelBrake",player,pedals,IntPtr.Add(pedals,4));
                    Row(w,"auto",target,speed,active?1:0,ramp?1:0,VF(pedals,0),VF(pedals,4));
                }
                VB(race,33,true);VB(vehicle,1319,false);VF(player,776,30);VF(vehicle,360,29.5f);Field(dtName).SetValue(null,1f/60);
                foreach(bool automatic in new bool[]{false,true}) foreach(float speed in new float[]{0,5,30,80})
                foreach(float accelerator in new float[]{-.2f,0,.5f,1,1.2f}) foreach(float brake in new float[]{0,.5f,1}) {
                    VB(inputManager,132,automatic);VF(inputManager,4,accelerator);VF(inputManager,8,brake);VF(pace,64,speed);
                    Call("gaPlayerLocal.tickInput",player);
                    Row(w,"manual",automatic?1:0,accelerator,brake,speed,30,29.5f,VF(player,88),VF(vehicle,544),VF(vehicle,548));
                }
                foreach(bool active in new bool[]{false,true}) foreach(bool ramp in new bool[]{false,true})
                foreach(bool hold in new bool[]{false,true}) foreach(float distance in new float[]{-120,-5,0,1.5f,30,150}) {
                    Call("gaSpring.{ctor}",IntPtr.Add(player,588),200f);VF(player,776,17);
                    for(int step=0;step<12;step++) {
                        float dt=step%2==0?1f/120:1f/30, paceSpeed=step<6?30:50, speed=step<8?35:20;
                        float offset=step%3==0?6:0,desired=step%4==0?10:0,vx=step%2==0?3:0,vy=step%3-1,vz=speed;
                        float sv=VF(player,596),sp=VF(player,600),previousTarget=VF(player,776);
                        VI(pace,4,active?1:0);VF(pace,64,paceSpeed);VF(vehicle,360,speed);VF(player,780,distance);
                        VF(player,696,desired);VF(component,268,offset);VF(player,84,hold?1:0);VB(vehicle,1319,ramp);
                        float gx=step%3==0?.6f:0,gy=step%3==0?-.8f:-1;V3(vehicle,704,gx,gy,0);
                        V3(body,304,vx,vy,vz);VI(body,216,2);VF(body,220,3);Field(dtName).SetValue(null,dt);
                        Call("gaPlayerLocal.tickTargetSpeed",player);
                        Row(w,"pace",active?1:0,dt,paceSpeed,speed,distance,desired,offset,80,hold?1:0,ramp?1:0,
                            vx,vy,vz,gx,gy,0,sv,sp,previousTarget,VF(player,596),VF(player,600),VF(player,776),VF(body,304),VF(body,308),VF(body,312),*(int*)((byte*)body+216),VF(body,220));
                    }
                }
                foreach(int flags in new int[]{0,0x8000,0x20000,0x28000}) foreach(bool ramp in new bool[]{false,true}) foreach(bool free in new bool[]{false,true}) {
                    VI(node0,240,flags);VB(vehicle,1319,ramp);VB(vehicle,1320,free);
                    Row(w,"adhesion",flags,ramp?1:0,free?1:0,(bool)Call("gaVehiclePhys.shouldStickToTrack",vehicle,false)?1:0);
                }
                foreach(float cone in new float[]{0,1,5,45,90}) foreach(float angle in new float[]{0,.1f,1,4.99f,5,45,89.99f,90,90.01f,135,179,180}) {
                    VF(parameters,68,cone);Row(w,"grip",cone,angle,(float)Call("gaVehiclePhys.phys_calcGrip",vehicle,angle));
                }
                foreach(bool both in new bool[]{false,true}) foreach(float l0 in new float[]{0,.25f,1}) foreach(float r0 in new float[]{0,.5f,1}) foreach(float t in new float[]{0,.25f,.75f,1}) {
                    VP(tracker,80,both?node1:IntPtr.Zero);VF(node0,208,l0);VF(node0,212,r0);VF(node1,208,.8f);VF(node1,212,.2f);VF(tracker,84,t);
                    Row(w,"lanes",both?1:0,l0,r0,.8f,.2f,t,(float)Call("gaNodeTracker.getNumLanes_DAT",tracker));
                }
                VP(tracker,80,node1);
                float[] tuning=Array.ConvertAll(File.ReadAllText(Path.Combine(input,"vehicle-friction.txt")).Split(new char[]{' ','\t','\n','\r'},StringSplitOptions.RemoveEmptyEntries),x=>float.Parse(x,Invariant));
                if(tuning.Length!=15)throw new IOException("Expected 15 original friction parameters");
                int[] offsets={64,68,72,76,80,84,88,92,96,100,112,116,208,212,252};
                for(int variant=0;variant<2;variant++) {
                    // First set is the installed base preset. The second makes
                    // each surface coefficient distinct to exercise dispatch.
                    float[] p=(float[])tuning.Clone();if(variant==1){for(int i=2;i<8;i++)p[i]=.2f+i*.15f;p[8]=.4f;p[9]=.9f;p[10]=.6f;p[11]=.3f;p[12]=2.5f;}
                    for(int i=0;i<p.Length;i++)VF(parameters,offsets[i],p[i]);
                    foreach(int surface in new int[]{0,1,6,7,9,13,99}) foreach(float throttle in new float[]{0,.5f,1}) foreach(float steer in new float[]{-1,0,1}) foreach(float drift in new float[]{0,.5f,1}) foreach(float brake in new float[]{0,1}) foreach(float lanes in new float[]{1,2,2.5f,3}) {
                        VI(wheel,84,surface);VF(gearbox,44,throttle);VF(vehicle,1072,steer);VF(vehicle,1104,drift);VF(gearbox,48,brake);
                        VF(node0,208,lanes-2);VF(node0,212,1);VF(tracker,84,0);
                        float[] row=new float[22];Array.Copy(p,row,15);row[15]=surface;row[16]=throttle;row[17]=steer;row[18]=drift;row[19]=brake;row[20]=lanes;row[21]=(float)Call("gaVehiclePhys.phys_calcLatFriction",vehicle,0);Row(w,"lateral",row);
                    }
                    foreach(bool parked in new bool[]{false,true}) foreach(float speed in new float[]{-20,0,30}) foreach(float drift in new float[]{0,.5f,1}) foreach(float brake in new float[]{0,.5f,1}) foreach(int contacts in new int[]{0,1,4}) {
                        VB(vehicle,578,parked);VF(vehicle,360,speed);VF(vehicle,1104,drift);VF(gearbox,48,brake);VI(vehicle,956,contacts);V3(body,304,speed*.2f,3,speed);
                        float[] row=new float[24];Array.Copy(p,row,15);row[15]=parked?1:0;row[16]=speed;row[17]=drift;row[18]=brake;row[19]=contacts;row[20]=speed*.2f;row[21]=3;row[22]=speed;row[23]=(float)Call("gaVehiclePhys.phys_calcForwardFriction",vehicle);Row(w,"forward",row);
                    }
                }
                foreach(float extra in new float[]{0,12}) foreach(float speed in new float[]{0,5,30,100}) foreach(float drift in new float[]{0,1}) foreach(float bias in new float[]{-.5f,0,.5f}) foreach(float steer in new float[]{-1,0,1}) foreach(float lateral in new float[]{-20,0,20}) {
                    VF(parameters,20,30);VF(parameters,44,6);VF(parameters,36,extra);VF(parameters,516,8);Call("gaVehicleParams.updateInterpolatedValues",parameters);
                    float fx=extra==0 && drift>0?.6f:0,fy=extra>0?.6f:0,fz=fx!=0 || fy!=0?.8f:1;
                    VF(vehicle,360,speed);VF(vehicle,1104,drift);VF(vehicle,1136,bias);VF(vehicle,1072,steer);VF(vehicle,564,.05f);V3(body,304,lateral,0,speed);V3(vehicle,32,fx,fy,fz);
                    Call("gaVehiclePhys.phys_calcSteerAngle",vehicle);
                    Row(w,"steer-angle",30,6,extra,8,speed,drift,bias,steer,.05f,lateral,0,speed,fx,fy,fz,VF(vehicle,1032));
                }
                float[] inputTuning=Array.ConvertAll(File.ReadAllText(Path.Combine(input,"vehicle-input.txt")).Split(new char[]{' ','\t','\n','\r'},StringSplitOptions.RemoveEmptyEntries),x=>float.Parse(x,Invariant));
                if(inputTuning.Length!=6)throw new IOException("Expected five input parameters and mass");
                int[] stateOffsets={1072,1076,1080,1084,1088,1092,1096,1100,1104,1108,1112,1132,416};
                foreach(float release in new float[]{inputTuning[3],Math.Abs(inputTuning[3])}) {
                    VF(parameters,16,inputTuning[0]);VF(parameters,164,inputTuning[1]);VF(parameters,172,inputTuning[2]);VF(parameters,176,release);VF(parameters,168,inputTuning[4]);
                    foreach(int offset in stateOffsets)VF(vehicle,offset,0);
                    for(int step=0;step<240;step++) {
                        float dt=step%2==0?1f/120:1f/60,accel=step<160?1:0,brake=step>=160?.5f:0,steer=step<40?-1:step<80?1:step<120?.3f:step<160?0:-.5f;
                        bool pressed=(step>=10 && step<36)||(step>=80 && step<91),disabled=step>=200 && step<211;
                        Field(dtName).SetValue(null,dt);VF(vehicle,544,accel);VF(vehicle,548,brake);VF(vehicle,552,steer);VF(vehicle,556,.25f);VB(vehicle,576,pressed);VB(vehicle,1303,disabled);
                        VF(vehicle,1092,step%80>=10 && step%80<50?.3f:0);VF(vehicle,1132,step%60<30?5:-5);
                        V3(vehicle,776,1,0,0);V3(vehicle,788,0,1,0);float tz=(step%5-2)*.6f;V3(vehicle,800,0,0,tz);
                        float[] row=new float[50];Array.Copy(inputTuning,row,5);row[3]=release;row[5]=dt;row[6]=accel;row[7]=brake;row[8]=steer;row[9]=.25f;row[10]=pressed?1:0;row[11]=disabled?1:0;
                        row[12]=1;row[16]=1;row[20]=tz;
                        for(int i=0;i<stateOffsets.Length;i++)row[21+i]=i==10?((byte*)vehicle)[1112]:VF(vehicle,stateOffsets[i]);
                        Call("gaVehiclePhys.phys_tickInputs",vehicle);
                        for(int i=0;i<stateOffsets.Length;i++)row[34+i]=i==10?((byte*)vehicle)[1112]:VF(vehicle,stateOffsets[i]);
                        row[47]=VF(vehicle,544);row[48]=VF(vehicle,548);row[49]=VF(vehicle,552);Row(w,"inputs",row);
                    }
                }
                IntPtr bodyTable=Alloc(128);VP(bodyTable,40,Marshal.GetFunctionPointerForDelegate(applyAt));VP(vehicle,0,bodyTable);
                // Actual native vtable targets from elPhysBody's constructor table
                // at RVA 0x5F982C. Invoke original machine code, not a callback formula.
                IntPtr image=Marshal.GetHINSTANCE(module);
                if(image==IntPtr.Zero || image==new IntPtr(-1))throw new IOException("Original image base unavailable");
                var originalForce=(ApplyAtFn)Marshal.GetDelegateForFunctionPointer(IntPtr.Add(image,0x3B660),typeof(ApplyAtFn));
                var originalTorque=(ApplyTorqueFn)Marshal.GetDelegateForFunctionPointer(IntPtr.Add(image,0x3BB50),typeof(ApplyTorqueFn));
                IntPtr forceInput=Alloc(12),forcePoint=Alloc(12),linearAccel=Alloc(12),angularAccel=Alloc(12),totalForce=Alloc(12),totalTorque=Alloc(12);
                for(int mode=0;mode<3;mode++) foreach(float inverseMass in new float[]{0,.001f,.25f})
                foreach(int flags in new int[]{0,1,2,3}) foreach(int activation in new int[]{1,2,4,5}) for(int variant=0;variant<4;variant++) {
                    V3(body,404,1,-2,3);V3(body,420,-4,5,-6);
                    for(int step=0;step<4;step++) {
                        float[] row=new float[45];row[0]=mode;row[1]=flags;row[2]=activation;row[3]=inverseMass;
                        row[4]=variant==0?0:1;row[5]=variant==1?0:.3f;row[6]=.7f;
                        row[7]=variant==2?0:.8f;row[8]=.4f;row[9]=variant==3?0:1;
                        float[] matrix={.2f,.03f,-.04f,.03f,.3f,.02f,-.04f,.02f,.4f};Array.Copy(matrix,0,row,10,9);
                        for(int i=0;i<3;i++) {VF(body,340+i*4,row[4+i]);VF(body,536+i*4,row[7+i]);for(int j=0;j<3;j++)VF(body,256+i*16+j*4,matrix[i*3+j]);row[19+i]=VF(body,404+i*4);row[22+i]=VF(body,420+i*4);}
                        row[25]=step==0?0:step==1?13.7f:-4.3f;row[26]=step==0?0:2.9f;row[27]=step==0?0:-7.1f;
                        row[28]=step==1?0:1.2f;row[29]=step==1?0:-.3f;row[30]=step==1?0:2.4f;
                        V3(forceInput,0,row[25],row[26],row[27]);V3(forcePoint,0,row[28],row[29],row[30]);
                        VI(body,204,flags);VI(body,216,activation);VF(body,220,3);VF(body,336,inverseMass);
                        if(mode==0)originalForce(vehicle,forceInput,forcePoint);else if(mode==1)originalTorque(vehicle,forceInput);
                        Call("elPhysBody.getTotalForce",vehicle,totalForce);Call("elPhysBody.getTotalTorque",vehicle,totalTorque);
                        Call("elPhysBody.convertLinearImpulseToAccel",vehicle,linearAccel,forceInput);
                        Call("elPhysBody.convertTorqueImpulseToAngAccel",vehicle,angularAccel,forceInput);
                        for(int i=0;i<3;i++){row[31+i]=VF(totalForce,i*4);row[34+i]=VF(totalTorque,i*4);row[37+i]=VF(linearAccel,i*4);row[40+i]=VF(angularAccel,i*4);}
                        row[43]=*(int*)((byte*)body+216);row[44]=VF(body,220);Row(w,"body-force",row);
                    }
                }
                VI(body,204,0);
                VP(bodyTable,36,Marshal.GetFunctionPointerForDelegate(applyTorque));
                IntPtr tireTorque=Alloc(12);V3(tireTorque,0,2,-3,4);
                for(int variant=0;variant<12;variant++) foreach(float travel in new float[]{0,.000999f,.001f,.4f})
                foreach(float yaw in new float[]{-7,-6,0,6,7}) {
                    VF(vehicle,1128,-.25f);
                    for(int step=0;step<12;step++) {
                        float dt=step%2==0?1f/120:1f/30,a=step%4==0?0:step%4==1?.2f:step%4==2?1:1.5f,b=step%3==0?.5f:0;
                        float ux=variant%3==0?0:variant%3==1?.6f:1,uy=variant%3==0?1:variant%3==1?.8f:0;
                        float rx=uy,ry=-ux,trackX=(step%6-2)*.5f,trackY=.1f;
                        float ax=ux*yaw+rx*.3f,ay=uy*yaw+ry*.3f,az=.7f;
                        float acceleration=step%3-1,lateral=step%4==0?-5:step%4==1?-4:step%4==2?4:5,drift=step%2==0?0:.5f;
                        float limit2=variant/3==0?0:40,limit3=variant/3==1?0:variant/3==2?20:40,lanes=step%4==0?1:step%4==1?2:step%4==2?2.5f:3;
                        int contacts=step%3==0?0:4;float latch=VF(vehicle,1128);
                        Field(dtName).SetValue(null,dt);VF(vehicle,1116,a);VF(vehicle,1120,b);VF(vehicle,1132,acceleration);VF(vehicle,356,lateral);VF(vehicle,1104,drift);VI(vehicle,1004,contacts);
                        VF(parameters,104,limit2);VF(parameters,108,limit3);VF(node0,208,lanes-2);VF(node0,212,1);VF(tracker,84,0);
                        V3(vehicle,20,ux,uy,0);V3(vehicle,8,rx,ry,0);V3(vehicle,776,trackX,trackY,0);V3(body,320,ax,ay,az);
                        VF(vehicle,1136,9);VI(body,216,2);VF(body,220,3);torqueCalls=0;Array.Clear(capturedTorque,0,3);
                        Call("gaVehiclePhys.phys_applyYawDamping",vehicle,travel,tireTorque);
                        Row(w,"yaw",dt,travel,a,b,latch,acceleration,lateral,drift,contacts,limit2,limit3,lanes,
                            ux,uy,0,rx,ry,0,trackX,trackY,0,ax,ay,az,2,-3,4,
                            VF(vehicle,1128),VF(vehicle,1136),VF(body,320),VF(body,324),VF(body,328),
                            capturedTorque[0],capturedTorque[1],capturedTorque[2],torqueCalls,*(int*)((byte*)body+216),VF(body,220));
                    }
                }
                foreach(float lateral in new float[]{-1,1,1.001f,6.75f,12.5f})
                foreach(float speed in new float[]{-20,-13.4112f,-10,-.250001f,-.25f,-.249999f,0,2.3f,9.999f,10,13.411199f,13.4112f,20})
                foreach(float pedal in new float[]{-1,-.900001f,-.9f,0,.9f,.900001f,1})
                foreach(float drift in new float[]{0,.5f,.500001f}) foreach(int contacts in new int[]{0,1,4}) foreach(int gear in new int[]{0,1}) {
                    VF(vehicle,360,speed);VF(vehicle,1080,pedal);VF(vehicle,1104,drift);VI(vehicle,1004,contacts);VI(gearbox,28,gear);
                    V3(vehicle,364,9,9,9);Call("gaVehiclePhys.phys_calcSkid",vehicle,lateral);
                    Row(w,"skid",lateral,speed,pedal,drift,contacts,gear,VF(vehicle,364),VF(vehicle,368),VF(vehicle,372));
                }
                VP(bodyTable,32,Marshal.GetFunctionPointerForDelegate(applyAt));
                IntPtr boostDirection=Alloc(12);V3(boostDirection,0,0,.6f,.8f);
                foreach(int contacts in new int[]{0,1,2,4}) foreach(float coefficient in new float[]{0,1.1f,3})
                foreach(float lateral in new float[]{-5,0,5}) foreach(float response in new float[]{-100,0,4})
                foreach(float vx in new float[]{0,.1f,.9f,1,4,30}) foreach(bool banked in new bool[]{false,true}) {
                    float rx=banked?.8f:1,ry=banked?.6f:0,vz=vx<1?0:12,mass=inputTuning[5];
                    VI(vehicle,1004,contacts);VF(parameters,300,coefficient);VF(vehicle,356,lateral);VF(vehicle,144,mass);
                    V3(vehicle,8,rx,ry,0);V3(vehicle,392,response,2,0);V3(body,304,vx,0,vz);V3(vehicle,32,0,0,1);
                    impulseCalls=0;Array.Clear(capturedImpulse,0,3);Call("gaVehiclePhys.phys_applySkidSpeedup",vehicle,boostDirection);
                    Row(w,"skid-force",mass,coefficient,lateral,contacts,rx,ry,0,response,2,0,vx,0,vz,0,0,1,0,.6f,.8f,
                        capturedImpulse[0],capturedImpulse[1],capturedImpulse[2],impulseCalls);
                }
                foreach(float torque in new float[]{0,1000}) foreach(float speed in new float[]{.01f,1,30}) foreach(float offset in new float[]{-24,0,24}) foreach(float boost in new float[]{0,8}) foreach(int contacts in new int[]{0,4}) foreach(bool full in new bool[]{false,true}) {
                    float mass=inputTuning[5],dt=1f/60;VF(vehicle,144,mass);VF(vehicle,360,speed);VF(gearbox,36,torque);VF(player,88,offset);VF(player,92,boost);VI(vehicle,1004,contacts);VB(vehicle,1300,full);V3(vehicle,32,0,.6f,.8f);V3(vehicle,312,0,1,0);Field(dtName).SetValue(null,dt);
                    impulseCalls=0;Array.Clear(capturedImpulse,0,3);Call("gaVehiclePhys.phys_tickDrive",vehicle);
                    Row(w,"drive-impulse",mass,torque,speed,offset,boost,contacts,full?1:0,dt,0,.6f,.8f,0,1,0,VF(vehicle,1028),VF(vehicle,692),VF(vehicle,696),VF(vehicle,700),capturedImpulse[0],capturedImpulse[1],capturedImpulse[2],impulseCalls,VF(vehicle,1068));
                }
                // Execute the complete original tire method with only suspension
                // enabled. The native force call is observed and then forwarded.
                float[] suspensionTuning=Array.ConvertAll(File.ReadAllText(Path.Combine(input,"vehicle-suspension.txt")).Split(new char[]{' ','\t','\n','\r'},StringSplitOptions.RemoveEmptyEntries),x=>float.Parse(x,Invariant));
                if(suspensionTuning.Length!=2)throw new IOException("Expected original suspension parameters");
                for(int i=0;i<2048;i++)((byte*)vehicle)[i]=0;
                for(int i=0;i<1024;i++){((byte*)parameters)[i]=0;((byte*)body)[i]=0;}
                for(int i=0;i<128;i++)((byte*)wheel)[i]=0;
                VP(vehicle,0,bodyTable);VP(vehicle,120,body);VP(vehicle,328,holder);VP(vehicle,332,parameters);VP(vehicle,824,gearbox);
                VP(vehicle,420,wheelPointers);VP(vehicle,424,IntPtr.Add(wheelPointers,4));VP(wheelPointers,0,wheel);
                VP(bodyTable,32,Marshal.GetFunctionPointerForDelegate(captureWheelForce));VP(bodyTable,36,Marshal.GetFunctionPointerForDelegate(originalTorque));forwardWheelForce=originalForce;
                VF(parameters,68,5);VF(parameters,120,.5f);VF(parameters,124,suspensionTuning[0]);VF(parameters,128,suspensionTuning[1]);
                V3(body,340,1,1,1);V3(body,536,1,1,1);V3(body,256,1,0,0);V3(body,272,0,1,0);V3(body,288,0,0,1);
                VF(body,336,1/inputTuning[5]);VF(vehicle,144,inputTuning[5]);VI(vehicle,1004,1);V3(vehicle,704,0,-1,0);VF(wheel,64,.4f);V3(wheel,40,1.2f,-.3f,2.4f);
                float gradient=(float)Field("?A0xc20f0743.s_cosMaxGradientBegin").GetValue(null);
                foreach(bool stick in new bool[]{false,true}) foreach(float smoothing in new float[]{0,5,120})
                for(int orientation=0;orientation<4;orientation++) foreach(float vy in new float[]{-10,0,10}) {
                    VF(wheel,36,7);
                    foreach(float compression in new float[]{-.3f,-.199001f,-.199f,-.198999f,-.05f,0,.05f,.3f}) {
                        float ux=orientation==1?.6f:0,uy=orientation==1?.8f:1;
                        float nx=orientation==2?.6f:orientation==3?1:ux,ny=orientation==2?.8f:orientation==3?0:uy;
                        float previous=VF(wheel,36),dt=1f/60,mass=inputTuning[5];
                        V3(vehicle,8,uy,-ux,0);V3(vehicle,20,ux,uy,0);V3(vehicle,32,0,0,1);V3(vehicle,312,ux,uy,0);
                        V3(wheel,4,nx,ny,0);VF(wheel,52,compression);VF(parameters,408,smoothing);VI(node0,240,stick?0x8000:0x20000);
                        V3(body,304,2,vy,3);V3(body,320,.2f,-.3f,.4f);V3(body,404,0,0,0);V3(body,420,0,0,0);
                        VF(vehicle,1116,0);VF(vehicle,1120,0);VF(vehicle,1136,0);Field(dtName).SetValue(null,dt);wheelForceCalls=0;
                        Call("gaVehiclePhys.phys_applySuspAndTyreForces_HI",vehicle);
                        if(wheelForceCalls!=1)throw new IOException("Expected one isolated suspension force");
                        Row(w,"suspension",dt,mass,suspensionTuning[0],suspensionTuning[1],smoothing,compression,.4f,stick?1:0,previous,gradient,
                            ux,uy,0,ux,uy,0,nx,ny,0,wheelPoint[0],wheelPoint[1],wheelPoint[2],2,vy,3,.2f,-.3f,.4f,
                            VF(vehicle,1228),VF(wheel,36),wheelForce[0],wheelForce[1],wheelForce[2],VF(body,404),VF(body,408),VF(body,412),VF(body,420),VF(body,424),VF(body,428));
                    }
                }
                // Combined four-wheel forces. Steering is zero and drift inertia
                // is disabled here; the test isolates force assembly/distribution.
                IntPtr[] wheels={wheel,Alloc(128),Alloc(128),Alloc(128)};
                for(int i=0;i<4;i++)VP(wheelPointers,i*4,wheels[i]);
                VP(vehicle,424,IntPtr.Add(wheelPointers,16));
                for(int i=72;i<=116;i+=4)VF(parameters,i,1);
                VF(parameters,104,0);VF(parameters,108,0);VF(parameters,140,0);
                foreach(float rearBias in new float[]{0,.5f,1}) foreach(bool stick in new bool[]{false,true})
                for(int orientation=0;orientation<4;orientation++) for(int scenario=0;scenario<12;scenario++) {
                    float[] row=new float[96];
                    row[0]=1f/60;row[1]=inputTuning[5];row[2]=suspensionTuning[0];row[3]=suspensionTuning[1];
                    row[4]=scenario%2==0?0:5;row[5]=stick?1:0;row[6]=rearBias;row[7]=scenario%3==0?0:scenario%3==1?1000:-200;
                    row[8]=scenario%2==0?0:1.6f;row[9]=5;row[10]=scenario%4==0?400:scenario%4==1?4000:100;
                    row[11]=scenario%3*.5f;row[12]=scenario%2;row[13]=scenario%3==0?0:.2f;row[14]=scenario%4==0?1:0;
                    row[15]=scenario%3==0?1:0;row[16]=scenario%3==0?1:scenario%3==1?.3f:0;row[17]=scenario%5;
                    float ux=orientation==1?.6f:0,uy=orientation==0?1:orientation==3?.5f:.8f,uz=orientation==2?.6f:orientation==3?.8660254f:0;
                    float rx=orientation==1?.8f:1,ry=orientation==1?-.6f:0,fy=orientation>=2?-uz:0,fz=orientation>=2?uy:1;
                    row[18]=rx;row[19]=ry;row[21]=ux;row[22]=uy;row[23]=uz;row[25]=fy;row[26]=fz;row[28]=-1;
                    row[30]=scenario%4==0?-10:scenario%4==1?-.01f:scenario%4==2?.01f:10;row[31]=scenario%3-1;row[32]=scenario%3==0?.1f:15;
                    V3(vehicle,8,rx,ry,0);V3(vehicle,20,ux,uy,uz);V3(vehicle,32,0,fy,fz);V3(vehicle,312,ux,uy,uz);V3(vehicle,704,0,-1,0);
                    V3(body,304,row[30],row[31],row[32]);V3(body,320,0,0,0);V3(body,404,0,0,0);V3(body,420,0,0,0);
                    VF(parameters,408,row[4]);VF(parameters,120,rearBias);VF(parameters,284,row[8]);VF(parameters,64,row[10]);VF(parameters,412,row[12]);
                    VF(gearbox,44,row[11]);VF(gearbox,48,0);VF(vehicle,352,row[13]);VF(vehicle,1040,row[14]);VF(vehicle,1140,row[15]);VF(vehicle,1152,row[16]);
                    VF(vehicle,1028,row[7]);VI(vehicle,1004,(int)row[17]);VF(vehicle,1116,0);VF(vehicle,1120,0);VF(vehicle,1136,0);
                    VI(node0,240,stick?0x8000:0x20000);Field(dtName).SetValue(null,row[0]);wheelForceCalls=0;
                    for(int i=0;i<4;i++) {
                        int p=36+i*9;int phase=(scenario+i)%4;
                        row[p]=phase==0?-.3f:phase==1?-.05f:phase==2?0:.08f;row[p+1]=.4f;row[p+2]=7+i;
                        row[p+3]=ux;row[p+4]=uy;row[p+5]=uz;
                        if(scenario%4==0 && i==3){row[p+3]=-ux;row[p+4]=-uy;row[p+5]=-uz;}
                        VF(wheels[i],52,row[p]);VF(wheels[i],64,row[p+1]);VF(wheels[i],36,row[p+2]);
                        V3(wheels[i],4,row[p+3],row[p+4],row[p+5]);V3(wheels[i],40,i%2==0?-1.2f:1.2f,-.3f,i<2?2.4f:-2.4f);
                    }
                    Call("gaVehiclePhys.phys_applySuspAndTyreForces_HI",vehicle);
                    if(wheelForceCalls!=4+((int)row[17]>=2?1:0))throw new IOException("Unexpected combined force call count");
                    for(int i=0;i<4;i++){int p=36+i*9;for(int j=0;j<3;j++){row[p+6+j]=allWheelPoints[i,j];row[73+i*4+j]=allWheelForces[i,j];}row[72+i*4]=VF(wheels[i],36);}
                    for(int i=0;i<3;i++){row[88+i]=VF(body,404+i*4);row[91+i]=VF(body,420+i*4);}row[94]=VF(vehicle,1228);row[95]=VF(vehicle,1232);
                    Row(w,"tires",row);
                }
            }
        } finally {SetGlobal(raceName,oldRace);SetGlobal(inputName,oldInput);SetGlobal(deviceName,oldDevice);Field(dtName).SetValue(null,oldDt);}
        Console.WriteLine("CAPTURE=original vehicle behavior complete");
    }
}
