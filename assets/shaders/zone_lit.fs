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

float luminance(vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec2 rotate2(vec2 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

void main()
{
    vec2 materialUv = fragTexCoord * textureScale;
    vec4 texel = texture(texture0, materialUv);

    vec3 normal = normalize(fragNormal);
    float waterFromTexture = smoothstep(0.10, 0.38, texel.b - texel.r + 0.08) * smoothstep(0.06, 0.26, texel.g - texel.r + 0.03);
    float waterFromTint = smoothstep(0.08, 0.34, colDiffuse.b - colDiffuse.r + 0.04) * smoothstep(0.02, 0.24, colDiffuse.g - colDiffuse.r + 0.02);
    float waterMaterial = max(waterFromTexture, waterFromTint);
    vec2 surfacePlane = abs(normal.y) > 0.55 ? fragPosition.xz : vec2(fragPosition.x + fragPosition.z * 0.37, fragPosition.y);
    vec2 surfaceLocal = materialUv + surfacePlane * 0.085;
    vec2 swirlCenterA = vec2(0.33 + 0.18 * sin(time * 0.31), 0.48 + 0.17 * cos(time * 0.23));
    vec2 swirlCenterB = vec2(0.68 + 0.16 * cos(time * 0.29), 0.55 + 0.19 * sin(time * 0.21));
    vec2 pA = fract(surfaceLocal * 0.82 + vec2(sin(time * 0.17), cos(time * 0.15)) * 0.22) - swirlCenterA;
    vec2 pB = fract(surfaceLocal * 0.57 + vec2(0.17, 0.31) + vec2(cos(time * 0.13), sin(time * 0.19)) * 0.18) - swirlCenterB;
    float rA = length(pA);
    float rB = length(pB);
    float aA = atan(pA.y, pA.x);
    float aB = atan(pB.y, pB.x);
    float vortexA = sin(aA * 9.0 + rA * 74.0 - time * 3.10);
    float vortexB = sin(aB * -8.0 + rB * 86.0 + time * 2.45);
    float surfaceMask = smoothstep(0.96, 0.03, rA) * 1.10 + smoothstep(0.90, 0.03, rB) * 0.95;
    vec2 tangentA = normalize(vec2(-pA.y, pA.x) + vec2(0.0001));
    vec2 tangentB = normalize(vec2(-pB.y, pB.x) + vec2(0.0001));
    vec2 feedbackWarp = (tangentA * vortexA * smoothstep(0.92, 0.02, rA) + tangentB * vortexB * smoothstep(0.86, 0.02, rB)) * 0.46;
    vec2 stripeWarp = vec2(
        sin(surfaceLocal.y * 47.0 + time * 3.4 + vortexB * 2.5),
        cos(surfaceLocal.x * 39.0 - time * 2.7 + vortexA * 2.0)
    ) * 0.085;
    vec3 warpedA = texture(texture0, materialUv + feedbackWarp + stripeWarp).rgb;
    vec3 warpedB = texture(texture0, materialUv - feedbackWarp * 1.15 + rotate2(stripeWarp, 1.7)).rgb;
    vec3 warpedC = texture(texture0, materialUv + feedbackWarp * 1.85 + vec2(0.071, -0.047) * sin(time * 1.23)).rgb;
    vec3 warpedD = texture(texture0, materialUv - feedbackWarp * 2.40 + vec2(-0.083, 0.066) * cos(time * 0.97)).rgb;
    vec3 feedbackAverage = (warpedA + warpedB * 0.82 + warpedC * 0.72 + warpedD * 0.58) / 3.12;
    vec3 materialDiff = abs(feedbackAverage - texel.rgb);
    float diffEnergy = smoothstep(0.006, 0.19, luminance(materialDiff)) * clamp(surfaceMask, 0.0, 1.45);
    vec3 acidRaw = vec3(
        0.52 + 0.48 * sin(time * 3.90 + materialUv.x * 21.0 + fragPosition.y * 1.7 + vortexA * 2.0),
        0.52 + 0.48 * sin(time * 3.17 + materialUv.y * 25.0 + 2.1 + vortexB * 2.4),
        0.52 + 0.48 * sin(time * 3.51 + (materialUv.x + materialUv.y) * 18.5 + 4.2 + vortexA * vortexB * 3.0)
    );
    vec3 acid = mix(vec3(luminance(acidRaw)), acidRaw, 0.34);
    vec3 negativeTrace = vec3(1.0) - texel.rgb;
    vec3 diffColor = mix(materialDiff * 6.8, acid, 0.78) * (0.90 + diffEnergy * 4.60);
    diffColor += abs(warpedA - warpedD).gbr * 2.80 * diffEnergy;
    vec3 psychedelicSurface = mix(texel.rgb + diffColor, negativeTrace * acid + feedbackAverage * 1.65, diffEnergy * 0.42);
    texel.rgb = mix(texel.rgb, clamp(psychedelicSurface, 0.0, 1.0), clamp(diffEnergy * 1.95 * (1.0 - waterMaterial * 0.76), 0.0, 1.0));

    float rippleA = sin(surfacePlane.x * 19.0 + time * 1.8) * 0.5 + 0.5;
    float rippleB = sin((surfacePlane.x + surfacePlane.y) * 31.0 - time * 2.7) * 0.5 + 0.5;
    float rippleLine = smoothstep(0.82, 0.98, rippleA * rippleB);
    float oilBandA = sin(surfacePlane.x * 8.5 + surfacePlane.y * 13.0 + time * 0.55);
    float oilBandB = sin(surfacePlane.x * -15.0 + surfacePlane.y * 9.0 - time * 0.42);
    float oilFilm = smoothstep(0.18, 0.92, oilBandA * 0.5 + oilBandB * 0.5 + rippleA * 0.34);
    vec3 oilPrismRaw = vec3(
        0.50 + 0.50 * sin(oilBandA * 3.6 + time * 0.90),
        0.50 + 0.50 * sin(oilBandB * 3.2 + time * 0.72 + 2.1),
        0.50 + 0.50 * sin((oilBandA - oilBandB) * 2.7 + time * 0.82 + 4.2)
    );
    vec3 oilPrism = mix(vec3(luminance(oilPrismRaw)), oilPrismRaw, 0.38);
    vec3 reflectedSky = vec3(0.62, 0.74, 0.82);
    vec3 reflectedLamp = vec3(1.00, 0.91, 0.70);
    vec3 glossyWater = mix(vec3(0.006, 0.036, 0.070), reflectedSky, 0.58 + rippleA * 0.30);
    glossyWater += reflectedLamp * rippleLine * 1.15;
    glossyWater = mix(glossyWater, oilPrism, waterMaterial * oilFilm * 0.24);
    glossyWater += oilPrism * rippleLine * oilFilm * 0.14;
    texel.rgb = mix(texel.rgb, clamp(glossyWater, 0.0, 1.0), waterMaterial * 0.96);

    vec4 base = texel * colDiffuse * fragColor;

    vec2 texelStep = vec2(1.0) / vec2(textureSize(texture0, 0));
    float height = luminance(texel.rgb);
    float heightX = luminance(texture(texture0, materialUv + vec2(texelStep.x, 0.0)).rgb);
    float heightY = luminance(texture(texture0, materialUv + vec2(0.0, texelStep.y)).rgb);
    float relief = (abs(height - heightX) + abs(height - heightY)) * materialDepth;
    float crevice = smoothstep(0.035, 0.22, relief);
    float lowPore = smoothstep(0.54, 0.16, height) * materialDepth;
    base.rgb *= 1.0 - crevice * 0.46 - lowPore * 0.16;

    vec3 light = normalize(-lightDir);
    vec3 viewDir = normalize(viewPos - fragPosition);

    float diffuse = max(dot(normal, light), 0.0);
    float selfShadow = clamp(1.0 - crevice * 0.52 - lowPore * 0.22, 0.46, 1.0);
    float skyBounce = clamp(normal.y * 0.5 + 0.5, 0.0, 1.0);
    float rim = pow(1.0 - max(dot(viewDir, normal), 0.0), 2.8) * 0.12;

    float sideFalloff = pow(1.0 - abs(normal.y), 0.72);
    vec3 lit = base.rgb * ambientColor.rgb * (0.19 + skyBounce * 0.21);
    lit += base.rgb * lightColor.rgb * pow(diffuse, 1.72) * 1.18 * selfShadow;
    lit += lightColor.rgb * rim * sideFalloff;
    lit += mix(vec3(0.0), vec3(0.68, 0.84, 0.92) * (0.18 + rim * 0.85 + rippleLine * 0.42), waterMaterial);
    lit += oilPrism * oilFilm * waterMaterial * (0.045 + rim * 0.10 + rippleLine * 0.075);
    lit = mix(lit * 0.62, lit, smoothstep(0.08, 0.74, diffuse + rim));

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
            float bias = max(0.0012 * (1.0 - dot(normal, light)), 0.00035);
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
            lit *= 1.0 - shadow * (0.18 + diffuse * 0.70);
        }
    }

    float dist = length(viewPos - fragPosition);
    float fog = smoothstep(fogStart, fogEnd, dist);
    lit = mix(lit, fogColor.rgb, fog);

    finalColor = vec4(pow(lit, vec3(1.0 / 2.2)), base.a);
}
