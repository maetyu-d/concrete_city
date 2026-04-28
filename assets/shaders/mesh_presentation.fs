#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D feedbackTexture;
uniform vec2 resolution;
uniform float time;

out vec4 finalColor;

float luminance(vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    vec3 c = texture(texture0, uv).rgb;
    vec3 bloom = texture(texture0, uv + texel * vec2(2.5, 1.5)).rgb;
    bloom += texture(texture0, uv + texel * vec2(-2.5, -1.5)).rgb;
    bloom += texture(texture0, uv + texel * vec2(5.5, -3.5)).rgb;
    bloom += texture(texture0, uv + texel * vec2(-5.5, 3.5)).rgb;
    bloom *= 0.25;

    float glow = smoothstep(0.34, 0.86, luminance(bloom));
    vec3 color = c * 0.88 + bloom * glow * 0.34;
    color = mix(color, vec3(luminance(color)), 0.08);
    color = pow(max(color, vec3(0.0)), vec3(1.08));

    vec2 centered = uv * 2.0 - 1.0;
    float vignette = smoothstep(1.36, 0.18, dot(centered, centered));
    color *= 0.20 + vignette * 0.98;

    finalColor = vec4(clamp(color, 0.0, 1.0), 1.0) * fragColor;
}
