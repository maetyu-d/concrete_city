#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D feedbackTexture;
uniform vec2 resolution;
uniform float time;

out vec4 finalColor;

float hash21(vec2 p)
{
    p = fract(p * vec2(234.34, 435.67));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

float luminance(vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

void main()
{
    vec2 uv = fragTexCoord;
    vec2 texel = 1.0 / resolution;

    vec3 c = texture(texture0, uv).rgb;
    vec2 centeredUv = uv - vec2(0.5);
    vec2 feedbackDrift = vec2(sin(time * 0.26 + uv.y * 5.0), cos(time * 0.21 + uv.x * 4.5)) * texel * 12.0;
    vec2 feedbackUv = vec2(0.5) + centeredUv * 0.990 + feedbackDrift;
    vec2 feedbackUv2 = vec2(0.5) + centeredUv * 0.976 - feedbackDrift * 0.52 + vec2(texel.x * 12.0, -texel.y * 8.0);
    vec2 feedbackUv3 = vec2(0.5) + centeredUv * 1.010 + vec2(sin(time * 0.14), cos(time * 0.16)) * texel * 10.0;
    vec2 feedbackUv4 = vec2(0.5) + centeredUv * 1.020 + vec2(cos(time * 0.10 + uv.y * 2.2), sin(time * 0.12 + uv.x * 2.4)) * texel * 16.0;
    vec3 feedback = texture(feedbackTexture, feedbackUv).rgb;
    feedback += texture(feedbackTexture, feedbackUv2).rgb * 0.90;
    feedback += texture(feedbackTexture, feedbackUv3).rgb * 0.72;
    feedback += texture(feedbackTexture, feedbackUv4).rgb * 0.54;
    feedback /= 3.16;

    vec3 north = texture(texture0, uv + vec2(0.0, texel.y * 3.8)).rgb;
    vec3 south = texture(texture0, uv - vec2(0.0, texel.y * 3.8)).rgb;
    vec3 east = texture(texture0, uv + vec2(texel.x * 3.8, 0.0)).rgb;
    vec3 west = texture(texture0, uv - vec2(texel.x * 3.8, 0.0)).rgb;
    vec3 diagA = texture(texture0, uv + texel * vec2(3.2, 2.7)).rgb;
    vec3 diagB = texture(texture0, uv + texel * vec2(-3.4, -2.9)).rgb;

    vec3 soft = (c * 2.0 + north + south + east + west + diagA * 0.85 + diagB * 0.85) / 6.70;
    float edge = length((east - west) + (north - south));
    float luma = luminance(c);

    vec3 ghostA = texture(texture0, uv + vec2(texel.x * 14.5, -texel.y * 8.5)).rgb;
    vec3 ghostB = texture(texture0, uv + vec2(-texel.x * 24.0, texel.y * 13.0)).rgb;
    vec3 ghostC = texture(texture0, uv + vec2(texel.x * 35.0, texel.y * 5.0)).rgb;
    vec3 ghostD = texture(texture0, uv + vec2(-texel.x * 6.0, -texel.y * 30.0)).rgb;
    vec3 veilSample = (soft * 1.2 + ghostA * 1.2 + ghostB + ghostC * 0.92 + ghostD * 0.82) / 5.14;

    vec3 lifted = mix(c, soft, 0.72);
    lifted = mix(vec3(luma), lifted, 0.22);

    vec3 shadowWash = vec3(0.34, 0.42, 0.46);
    vec3 paper = vec3(0.90, 0.88, 0.82);
    vec3 ochre = vec3(0.74, 0.61, 0.45);
    vec3 coldBlue = vec3(0.55, 0.65, 0.70);

    vec3 tone = mix(shadowWash, paper, smoothstep(0.02, 0.82, luma));
    tone = mix(tone, ochre, smoothstep(0.46, 0.90, c.r - c.b + 0.18) * 0.22);
    tone = mix(tone, coldBlue, smoothstep(0.18, 0.72, c.b + c.g - c.r) * 0.26);

    vec3 ground = vec3(0.88, 0.865, 0.80);
    vec3 pigment = mix(lifted, tone, 0.82);
    pigment = mix(pigment, veilSample, 0.46);
    vec3 painted = mix(c, pigment, 0.84);
    painted = mix(painted, ground, 0.070);

    float paperGrain = hash21(floor(uv * resolution * 0.72) + floor(time * 0.35));
    float largeStain = valueNoise(uv * vec2(14.0, 8.0) + time * 0.055);
    float cloudyStain = valueNoise(uv * vec2(4.4, 3.0) - time * 0.045);
    float washCurrent = valueNoise(uv * vec2(2.2, 1.7) + vec2(time * 0.030, -time * 0.024));
    float bleachPatch = valueNoise(uv * vec2(3.0, 2.2) + vec2(-time * 0.018, time * 0.012));
    painted += (paperGrain - 0.5) * 0.082;
    painted = mix(painted, painted * (0.82 + largeStain * 0.22), 0.42);
    painted = mix(painted, ground, smoothstep(0.58, 1.0, cloudyStain) * 0.065);
    painted = mix(painted, mix(ground, painted, 0.74), smoothstep(0.38, 0.92, washCurrent) * 0.12);
    painted = mix(painted, ground + vec3(0.055, 0.045, 0.020), smoothstep(0.62, 0.96, bleachPatch) * 0.18);

    float feedbackLuma = luminance(feedback);
    float feedbackMask = smoothstep(0.010, 0.18, feedbackLuma) * (1.0 - smoothstep(0.985, 1.0, feedbackLuma));
    vec3 feedbackChroma = mix(vec3(feedbackLuma), feedback, 2.35);
    vec3 feedbackContrast = clamp((feedbackChroma - 0.38) * 1.82 + 0.38, 0.0, 1.0);
    vec3 feedbackRgbSplit = vec3(
        texture(feedbackTexture, feedbackUv + texel * vec2(3.0, -1.0)).r,
        texture(feedbackTexture, feedbackUv2).g,
        texture(feedbackTexture, feedbackUv3 + texel * vec2(-3.0, 1.0)).b
    );
    feedbackRgbSplit = mix(vec3(luminance(feedbackRgbSplit)), feedbackRgbSplit, 0.42);
    vec3 feedbackWash = mix(feedbackContrast, feedbackRgbSplit * vec3(1.04, 1.02, 1.08), 0.30);
    feedbackWash = mix(feedbackWash, feedbackWash.gbr, smoothstep(0.22, 0.82, feedbackLuma) * 0.10);
    painted = mix(painted, feedbackWash, feedbackMask * 1.18);

    float bloom = smoothstep(0.50, 0.95, luma);
    painted += mix(soft, ground, 0.38) * bloom * 0.25;

    float erasedEdge = smoothstep(0.05, 0.34, edge);
    painted = mix(painted, painted * 0.52 + vec3(0.18, 0.25, 0.28), erasedEdge * 0.32);

    float milkyBloom = smoothstep(0.04, 0.62, 1.0 - luma);
    painted = mix(painted, ground, milkyBloom * 0.090);

    float overexpose = smoothstep(0.34, 0.92, luma);
    painted = mix(painted, ground + vec3(0.055, 0.045, 0.025), overexpose * 0.18);

    vec2 centered = uv * 2.0 - 1.0;
    float vignette = smoothstep(1.48, 0.24, dot(centered, centered));
    painted *= 0.92 + vignette * 0.08;
    painted = mix(c, painted, 0.90);

    vec2 pixel = uv * resolution;
    float cellSize = 7.0;
    vec2 cell = floor(pixel / cellSize);
    float cellJitter = hash21(cell + floor(time * 2.0));
    vec2 cellUv = fract(pixel / cellSize) - 0.5;
    float cellToneNoise = valueNoise(cell * 0.13 + vec2(time * 0.08, -time * 0.05));
    float blockTone = valueNoise(floor(pixel / 34.0) * 0.71 + vec2(11.7, -3.2));
    float paintedLuma = luminance(painted);
    float posterTone = floor((1.0 - paintedLuma + cellToneNoise * 0.14 + blockTone * 0.22) * 5.0) / 5.0;
    float inkNeed = clamp(posterTone + erasedEdge * 0.28 - overexpose * 0.18, 0.0, 1.0);
    float dotRadius = mix(0.05, 0.55, pow(inkNeed, 0.82));
    vec2 dotOffset = (vec2(hash21(cell + 19.2), hash21(cell - 41.8)) - 0.5) * 0.16;
    dotOffset += vec2(sin(time * 0.70 + cell.y * 0.20), cos(time * 0.55 + cell.x * 0.16)) * 0.006;
    dotOffset += (cellJitter - 0.5) * 0.010;
    float halftoneDot = 1.0 - smoothstep(dotRadius, dotRadius + 0.055, length(cellUv - dotOffset));
    float brokenInk = hash21(cell * vec2(1.7, 2.3) + floor(time * 3.0));
    halftoneDot *= smoothstep(0.10, 0.30, inkNeed + brokenInk * 0.18);
    float scanFade = 0.90 + 0.10 * sin(pixel.y * 3.14159 / cellSize);
    vec3 ink = vec3(0.014, 0.015, 0.014);
    vec3 halftonePaper = vec3(0.965, 0.955, 0.905);
    vec3 halftoneLayer = mix(halftonePaper, ink, clamp(halftoneDot * scanFade, 0.0, 1.0));
    float halftoneStrength = 0.045 + smoothstep(0.10, 0.70, inkNeed) * 0.070;
    painted = mix(painted, halftoneLayer, halftoneStrength);

    vec2 rainUv = uv * vec2(resolution.x / resolution.y, 1.0);
    vec2 slantA = vec2(rainUv.x + rainUv.y * 0.42, rainUv.y);
    vec2 slantB = vec2(rainUv.x + rainUv.y * 0.72, rainUv.y);
    float rainSeedA = hash21(floor(vec2(slantA.x * 155.0, slantA.y * 9.0 - time * 35.0)));
    float rainSeedB = hash21(floor(vec2(slantB.x * 92.0 + 19.0, slantB.y * 6.0 - time * 23.0)));
    float rainColumnA = 1.0 - smoothstep(0.018, 0.055, abs(fract(slantA.x * 155.0 + rainSeedA * 0.22) - 0.5));
    float rainColumnB = 1.0 - smoothstep(0.012, 0.046, abs(fract(slantB.x * 92.0 + rainSeedB * 0.27) - 0.5));
    float rainDashA = smoothstep(0.05, 0.22, fract(slantA.y * 34.0 - time * 19.0 + rainSeedA));
    float rainDashB = smoothstep(0.08, 0.28, fract(slantB.y * 24.0 - time * 13.0 + rainSeedB));
    float rain = clamp(rainColumnA * rainDashA * smoothstep(0.20, 1.0, rainSeedA) + rainColumnB * rainDashB * smoothstep(0.12, 1.0, rainSeedB), 0.0, 1.0);

    float rainCurtain = valueNoise(vec2(uv.x * 18.0 + time * 0.18, uv.y * 5.0 - time * 1.6));
    float oilFlow = valueNoise(vec2(uv.x * 5.2 + sin(time * 0.17) * 0.6, uv.y * 10.0 - time * 0.42));
    float oilVeil = smoothstep(0.48, 0.95, oilFlow) * (0.18 + rainCurtain * 0.32);
    vec3 oilBlack = vec3(0.015, 0.018, 0.017);
    vec3 oilRainbowRaw = vec3(
        0.50 + 0.50 * sin(uv.y * 34.0 + time * 1.4),
        0.50 + 0.50 * sin(uv.y * 31.0 + uv.x * 9.0 + time * 1.1 + 2.1),
        0.50 + 0.50 * sin(uv.y * 28.0 - uv.x * 7.0 + time * 1.2 + 4.2)
    );
    vec3 oilRainbow = mix(vec3(luminance(oilRainbowRaw)), oilRainbowRaw, 0.28);
    painted = mix(painted, painted * 0.42 + oilBlack, oilVeil * 0.64);
    painted += oilRainbow * oilVeil * 0.060;
    painted += vec3(0.82, 0.88, 0.92) * rain * (0.20 + rainCurtain * 0.20);
    painted = mix(painted, painted * vec3(0.78, 0.86, 0.92), smoothstep(0.15, 0.90, rainCurtain) * 0.12);
    float finalLuma = luminance(painted);
    vec3 bleachWhite = vec3(0.93, 0.91, 0.84);
    painted = mix(painted, mix(vec3(finalLuma), painted, 0.34), 0.42);
    painted = mix(painted, bleachWhite, smoothstep(0.34, 1.0, finalLuma) * 0.32 + 0.12);

    finalColor = vec4(clamp(painted, 0.0, 1.0), 1.0) * fragColor;
}
