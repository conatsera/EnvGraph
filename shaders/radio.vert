#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
layout (std140, binding = 0) readonly uniform bufferVals {
    mat4 mvp;
    float fftBins;
    float windowWidth;
    float strengthOffset;
    uint currentTimePos;
} myBufferVals;
layout (location = 0) in float strength;
layout (location = 0) out vec4 outColor;

const vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);

void main() {
    //gl_PointSize = 16384 / myBufferVals.fftBins;
 //   gl_PointSize = 1.0;

    const float colorScaleFactor = 64 / log(myBufferVals.fftBins);
    const vec3 c = vec3(strength - myBufferVals.strengthOffset / colorScaleFactor, 1.0, 1.0);
    const vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    const vec3 color = c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);

    outColor = vec4(color, 1.0);

    const float fftToPixel = ((myBufferVals.windowWidth / (myBufferVals.fftBins*2)) * 0.05);
    const float offset = fftToPixel * (myBufferVals.fftBins);

    float timePos = (myBufferVals.currentTimePos - floor((gl_VertexIndex / 2) / myBufferVals.fftBins))/2;
    if (timePos <= 0)
        timePos += 1023/2;

    const float bin = mod(gl_VertexIndex, myBufferVals.fftBins*2);

    gl_Position = myBufferVals.mvp * vec4((bin * fftToPixel) - offset, strength - myBufferVals.strengthOffset, timePos, 1.0);
}