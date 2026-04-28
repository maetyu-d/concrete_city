#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec4 ambientColor;
uniform vec4 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float materialDepth;
uniform float textureScale;
uniform float time;
uniform mat4 lightVP;
uniform sampler2D shadowMap;
uniform int shadowMapResolution;
uniform int useShadowMap;

out vec4 finalColor;

float lineGrid(vec2 coord, float scale, float width)
{
    vec2 scaled = coord * scale;
    vec2 cell = abs(fract(scaled) - 0.5);
    vec2 fw = fwidth(scaled);
    vec2 line = 1.0 - smoothstep(vec2(width), vec2(width) + fw * 1.55, cell);
    return max(line.x, line.y);
}

float lineStripes(float coord, float scale, float width)
{
    float scaled = coord * scale;
    float cell = abs(fract(scaled) - 0.5);
    float fw = fwidth(scaled);
    return 1.0 - smoothstep(width, width + fw * 1.65, cell);
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 light = normalize(-lightDir);
    vec3 viewDir = normalize(viewPos - fragPosition);

    vec2 surfaceCoord = abs(normal.y) > 0.55
        ? fragPosition.xz
        : vec2(dot(fragPosition.xz, vec2(0.82, 0.57)), fragPosition.y);

    float wave = sin(surfaceCoord.x * 0.72 + time * 0.18) * 0.22
               + cos(surfaceCoord.y * 0.54 - time * 0.14) * 0.18;
    vec2 warpedCoord = surfaceCoord + normal.xz * 0.12 + vec2(wave, -wave * 0.55);

    float fineGrid = lineGrid(warpedCoord, 2.95, 0.018);
    float midGrid = lineGrid(warpedCoord + vec2(0.07, 0.02), 1.12, 0.014);
    float majorGrid = lineGrid(warpedCoord, 0.36, 0.010);
    float verticalComb = lineStripes(fragPosition.y + wave * 0.18, 1.85, 0.012) * (1.0 - abs(normal.y));
    float contour = lineStripes(fragPosition.y + sin(surfaceCoord.x * 0.45) * 0.20, 0.62, 0.010);
    float lineMask = clamp(fineGrid * 0.62 + midGrid * 0.34 + majorGrid * 0.80 + verticalComb * 0.42 + contour * 0.28, 0.0, 1.0);

    float diffuse = max(dot(normal, light), 0.0);
    float rim = pow(1.0 - max(dot(viewDir, normal), 0.0), 2.2);
    float heightGlow = smoothstep(0.2, 17.0, fragPosition.y);
    float peakGlow = smoothstep(8.0, 28.0, fragPosition.y);
    float grazing = pow(1.0 - abs(dot(viewDir, normal)), 2.0);

    vec3 deepBlue = vec3(0.004, 0.006, 0.018);
    vec3 midnight = vec3(0.012, 0.016, 0.045);
    vec3 violet = vec3(0.055, 0.046, 0.105);
    vec3 steelLine = vec3(0.42, 0.48, 0.72);
    vec3 creamLine = vec3(1.00, 0.88, 0.66);
    vec3 iceLine = vec3(0.80, 0.90, 1.00);

    vec3 fill = mix(deepBlue, midnight, clamp(diffuse * 0.65 + heightGlow * 0.22, 0.0, 1.0));
    fill = mix(fill, violet, grazing * 0.18 + peakGlow * 0.10);

    vec3 lineColor = mix(steelLine, creamLine, clamp(diffuse * 0.95 + peakGlow * 0.55, 0.0, 1.0));
    lineColor = mix(lineColor, iceLine, rim * 0.36);
    float lineLight = lineMask * (0.46 + diffuse * 0.70 + rim * 0.92 + peakGlow * 0.64);

    if (useShadowMap == 1)
    {
        vec4 lightSpace = lightVP * vec4(fragPosition, 1.0);
        lightSpace.xyz /= lightSpace.w;
        lightSpace.xyz = lightSpace.xyz * 0.5 + 0.5;

        if (lightSpace.x >= 0.0 && lightSpace.x <= 1.0 &&
            lightSpace.y >= 0.0 && lightSpace.y <= 1.0 &&
            lightSpace.z >= 0.0 && lightSpace.z <= 1.0)
        {
            vec2 texelSize = vec2(1.0 / float(shadowMapResolution));
            float bias = max(0.0015 * (1.0 - dot(normal, light)), 0.00045);
            float shadow = 0.0;

            for (int x = -1; x <= 1; x++)
            {
                for (int y = -1; y <= 1; y++)
                {
                    float sampleDepth = texture(shadowMap, lightSpace.xy + texelSize * vec2(x, y)).r;
                    shadow += (lightSpace.z - bias > sampleDepth) ? 1.0 : 0.0;
                }
            }

            shadow /= 9.0;
            lineLight *= 1.0 - shadow * 0.42;
            fill *= 1.0 - shadow * 0.22;
        }
    }

    vec3 lit = fill + lineColor * lineLight;
    lit += creamLine * pow(rim, 3.0) * 0.34;
    lit += iceLine * peakGlow * lineMask * 0.28;

    float dist = length(viewPos - fragPosition);
    float fog = smoothstep(fogStart, fogEnd, dist);
    lit = mix(lit, vec3(0.0, 0.0, 0.004), fog * 0.94);

    finalColor = vec4(pow(clamp(lit, 0.0, 1.0), vec3(1.0 / 2.2)), fragColor.a * colDiffuse.a);
}
