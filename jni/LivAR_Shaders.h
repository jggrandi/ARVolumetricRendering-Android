/*==============================================================================


==============================================================================*/

#ifndef _QCAR_CUBE_SHADERS_H_
#define _QCAR_CUBE_SHADERS_H_


static const char gVertexShader[] = 
    "attribute vec3 vertexPos;\n"
    "attribute vec3 texCoord;\n"
    "uniform mat4 modelViewInverse;\n"
    "uniform mat4 mvp;\n"
    "uniform float sliceDistance;\n"
    "varying mediump vec3 v_texcoord;\n"
    "varying mediump vec4 v_texcoord2;\n"
    "void main() {\n"
    "   gl_Position = mvp * vec4(vertexPos,1.0);\n"
    "   v_texcoord = texCoord;\n"
    "   vec4 vPosition = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "   vPosition = modelViewInverse*vPosition;\n"
    "   vec4 vDir = vec4(0.0, 0.0, -1.0, 1.0);\n"
    "   vDir = normalize(modelViewInverse*vDir);\n"
    "   vec4 eyeToVert = normalize(vec4(vertexPos,1.0) - vPosition);\n"
    "   float t = sliceDistance/dot(vDir, eyeToVert);\n"
    "   eyeToVert = eyeToVert*t;\n"
    "   vec4 sB = vec4(texCoord,1.0) - eyeToVert;\n"
    "   v_texcoord2 = sB;\n"
    "}\n";

static const char gLowQualFragShader[] = 
    // Low Quality Fragment Shader
    "uniform sampler2D volumeSampler;\n"
    "uniform sampler2D lookupTableSampler;\n"
    "varying mediump vec3 v_texcoord;\n"
    "varying mediump vec4 v_texcoord2;\n"
    "void main() {\n"
    "   mediump vec2 texcoord;\n"
    "   texcoord.x = v_texcoord.x*0.125;\n"
    "   texcoord.y = v_texcoord.y*0.125;\n"
    "   int slice = int(v_texcoord.z);\n"
    "   int line = int(slice/8);\n"
    "   int row = slice-line*8;\n"
    "   texcoord.x = texcoord.x + float(row)*0.125;\n"
    "   texcoord.y = texcoord.y + float(line)*0.125;\n"
    "   mediump vec2 texCol;\n"
    "   texCol.x = texture2D(volumeSampler,texcoord.xy).x;\n"
    "   texCol.y = texCol.x;\n"
    "   mediump vec4 lookup = texture2D(lookupTableSampler,texCol.xy);\n"
    "   gl_FragColor = lookup;\n"
    "}\n";  

static const char gMediumQualFragShader[] = 
    // Medium Quality Fragment Shader
    "uniform sampler2D volumeSampler;\n"
    "uniform sampler2D lookupTableSampler;\n"
    "varying mediump vec3 v_texcoord;\n"
    "varying mediump vec4 v_texcoord2;\n"
    "void main()\n"
    "{\n"
        // code for a 8x8 slice img pre-integration classification
        "mediump vec2 texcoord;\n"
        "texcoord.x = v_texcoord.x*0.125;\n"
        "texcoord.y = v_texcoord.y*0.125;\n"
        "int slice = int(v_texcoord.z);\n"
        "int line = int(slice/8);\n"
        "int row = slice-line*8;\n"
        "texcoord.x = texcoord.x + float(row)*0.125;\n"
        "texcoord.y = texcoord.y + float(line)*0.125;\n"
        "mediump vec2 texcoord2;\n"
        "texcoord2.x = v_texcoord2.x*0.125;\n"
        "texcoord2.y = v_texcoord2.y*0.125;\n"
        "slice = int(v_texcoord2.z);\n"
        "line = int(slice/8);\n"
        "row = slice-line*8;\n" 
        "texcoord2.x = texcoord2.x + float(row)*0.125;\n"
        "texcoord2.y = texcoord2.y + float(line)*0.125;\n"
        // code with pre-integration table and no filtering in z
        "mediump vec2 texCol;\n" 
        "texCol.x = texture2D(volumeSampler,texcoord.xy).x;\n"
        "texCol.y = texture2D(volumeSampler,texcoord2.xy).x;\n"
        "mediump vec4 lookup = texture2D(lookupTableSampler,texCol.xy);\n"
        "gl_FragColor = lookup;\n"  
    "}\n";

static const char gHighQualFragShader[] = 
    // High Quality Fragment Shader
    "uniform sampler2D volumeSampler;\n"
    "uniform sampler2D lookupTableSampler;\n"
    "varying mediump vec3 v_texcoord;\n"
    "varying mediump vec4 v_texcoord2;\n"
    "void main(){\n"
    "mediump vec2 texcoord3;\n"
    "mediump vec2 texcoord4;\n"
    "mediump vec2 texcoord;\n"
    "texcoord.x = v_texcoord.x*0.125;\n"
    "texcoord.y = v_texcoord.y*0.125;\n"
    "int slice = int(v_texcoord.z);\n"
    "int line = int(slice/8);\n"
    "int row = slice-line*8;\n"
    "texcoord.x = texcoord.x + float(row)*0.125;\n"
    "texcoord.y = texcoord.y + float(line)*0.125;\n"
    

    "mediump float weight1;\n"
    "if (float(slice)>v_texcoord.z){\n"
    "   weight1 = float(slice)-v_texcoord.z;\n"
    "   slice -=1;\n"
    "}\n"
    "else {\n"
    "   weight1 = v_texcoord.z-float(slice);\n"
    "   slice+=1;\n"
    "}\n"
    "line = int(slice/8);\n"
    "row = slice-line*8;\n"
    "texcoord3.x = v_texcoord.x*0.125 + float(row)*0.125;\n"
    "texcoord3.y = v_texcoord.y*0.125 + float(line)*0.125;\n"
    

    "mediump vec2 texcoord2;\n"
    "texcoord2.x = v_texcoord2.x*0.125;\n"
    "texcoord2.y = v_texcoord2.y*0.125;\n"
    "slice = int(v_texcoord2.z);\n"
    "line = int(slice/8);\n"
    "row = slice-line*8;\n" 
    "texcoord2.x = texcoord2.x + float(row)*0.125;\n"
    "texcoord2.y = texcoord2.y + float(line)*0.125;\n"
        
    "mediump float weight2;\n"
    "if (float(slice)>v_texcoord2.z){\n"
    "   weight2 = float(slice)-v_texcoord2.z;\n"
    "   slice -=1;\n"
    "}\n"
    "else {\n"
    "   weight2 = v_texcoord2.z-float(slice);\n"
    "   slice+=1;\n"
    "}\n"
    "line = int(slice/8);\n"
    "row = slice-line*8;\n"
    "texcoord4.x = v_texcoord2.x*0.125 + float(row)*0.125;\n"
    "texcoord4.y = v_texcoord2.y*0.125 + float(line)*0.125;\n"

    // code with pre-integration table and filtering on Z\n"

    "mediump vec2 texCol;\n" 
    "texCol.x = texture2D(volumeSampler,texcoord.xy).x;\n"
    "texCol.y = texture2D(volumeSampler,texcoord2.xy).x;\n"
    "mediump vec2 texCol2;\n" 
    "texCol2.x = texture2D(volumeSampler,texcoord3.xy).x;\n"
    "texCol2.y = texture2D(volumeSampler,texcoord4.xy).x;\n"
    
    "mediump vec2 lerpTex; \n"
    "lerpTex.x = mix(texCol.x,texCol2.x,weight1);\n"
    "lerpTex.y = mix(texCol.y,texCol2.y,weight2);\n"

    "mediump vec4 lookup = texture2D(lookupTableSampler,lerpTex.xy);\n"

    "gl_FragColor = lookup;\n"
    "}\n";


#endif // _QCAR_CUBE_SHADERS_H_
