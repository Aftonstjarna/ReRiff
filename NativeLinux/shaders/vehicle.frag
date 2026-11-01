#version 450
layout(location=0) in vec3 worldPosition;
layout(location=1) in vec3 worldNormal;
layout(location=2) in vec3 baseColor;
layout(location=3) in vec2 surface;
layout(location=4) in vec3 textureUV;
layout(set=2,binding=0) uniform sampler2DArray surfaceTextures;
layout(set=3,binding=0) uniform Lighting { vec4 eye; } lighting;
layout(location=0) out vec4 fragment;
void main() {
    vec3 albedo=baseColor*texture(surfaceTextures,textureUV).rgb;
    vec3 n=normalize(worldNormal);
    if (!gl_FrontFacing) n=-n;
    vec3 v=normalize(lighting.eye.xyz-worldPosition);
    vec3 l=normalize(vec3(-0.4,0.85,0.6));
    vec3 h=normalize(v+l);
    float ndl=max(dot(n,l),0.0), ndv=max(dot(n,v),0.001);
    float ndh=max(dot(n,h),0.0), vdh=max(dot(v,h),0.0);
    float metal=surface.x, rough=max(surface.y,0.12);
    float a=rough*rough, a2=a*a;
    float d=a2/(3.14159265*pow(ndh*ndh*(a2-1.0)+1.0,2.0));
    float k=pow(rough+1.0,2.0)/8.0;
    float g=(ndv/(ndv*(1.0-k)+k))*(ndl/(ndl*(1.0-k)+k));
    vec3 f0=mix(vec3(0.04),albedo,metal);
    vec3 f=f0+(1.0-f0)*pow(1.0-vdh,5.0);
    vec3 specular=d*g*f/max(4.0*ndl*ndv,0.001);
    vec3 diffuse=(1.0-f)*(1.0-metal)*albedo/3.14159265;
    float ambient=0.22+0.22*max(n.y,0.0);
    vec3 light=albedo*ambient+(diffuse+specular)*ndl*2.5;
    // Approximate a broad studio reflection for metal without an environment map.
    light+=f0*pow(max(dot(reflect(-v,n),normalize(vec3(0.2,1.0,-0.4))),0.0),12.0)*0.6;
    light=light/(light+vec3(1.0));
    fragment=vec4(pow(light,vec3(1.0/2.2)),1.0);
}
