// CONSTANT BUFFERS
// b0 is used by vertex shader for viewProjection/cameraPosition/time
cbuffer SkyPropertiesBuffer : register(b1)
{
    float3 sunDirection;
    float sunIntensity;
    float3 sunColor;
    float sunSize;
    float3 moonDirection;
    float moonIntensity;
    float3 moonColor;
    float moonSize;

    float3 zenithColor;      // Sky color at top (day)
    float timeOfDay;         // 0.0 = midnight, 0.5 = noon, 1.0 = midnight
    float3 horizonColor;     // Sky color at horizon (day)
    float padding2;          // Alignment padding
    float3 nightZenithColor; // Sky color at night
    float padding3;          // Alignment padding
    float3 nightHorizonColor;
    float starIntensity;

    float atmosphereThickness;
    float rayleighScattering; // Blue light scattering
    float mieScattering;      // Haze/fog scattering
    float padding1;
};

// TEXTURES & SAMPLERS
Texture2D cloudTexture : register(t0);
Texture2D starTexture  : register(t1);
Texture2D moonTexture  : register(t2);

SamplerState linearSampler : register(s0);

// INPUT
struct PixelInput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POS;
    float3 viewDirection : VIEW_DIR;
    float2 texCoord      : TEXCOORD;
};

// CONSTANTS
static const float PI = 3.14159265359;

// HELPER FUNCTIONS

// Atmospheric scattering (simplified Rayleigh + Mie)
float3 AtmosphericScattering(float3 viewDir, float3 sunDir)
{
    float cosTheta = dot(viewDir, sunDir);

    // Rayleigh scattering (blue sky)
    float rayleigh = rayleighScattering * (1.0 + cosTheta * cosTheta);

    // Mie scattering (sun glow, haze)
    float g = 0.76; // Anisotropy factor
    float denominator = 1.0 + g * g - 2.0 * g * cosTheta;

    // FIX: Ensure denominator is positive before using pow
    denominator = max(abs(denominator), 0.0001); // Prevent negative and division by zero

    float mie = mieScattering * (1.0 - g * g) / pow(denominator, 1.5);

    return float3(rayleigh, rayleigh * 0.8, rayleigh * 0.6) +
           float3(mie, mie * 0.9, mie * 0.8);
}

// Day/night blend factor
float GetDayNightBlend(float timeOfDay)
{
    // Smooth transitions at sunrise and sunset
    float sunrise = 0.25;  // 6 AM
    float sunset = 0.75;   // 6 PM

    if (timeOfDay < sunrise)
    {
        // Night to sunrise - smooth transition
        return smoothstep(0.15, sunrise, timeOfDay);
    }
    else if (timeOfDay < sunset)
    {
        // Full day
        return 1.0;
    }
    else
    {
        // Sunset to night - smooth transition
        return smoothstep(0.85, sunset, timeOfDay);
    }
}

// Draw sun/moon disk
float DrawCelestialBody(float3 viewDir, float3 bodyDir, float size)
{
    float dist = distance(viewDir, bodyDir);
    float disk = smoothstep(size, size * 0.9, dist);

    // Add glow
    float glow = exp(-dist * 10.0) * 0.3;

    return saturate(disk + glow);
}

// Stars (only visible at night)
float3 DrawStars(float3 viewDir, float nightFactor)
{
    if (nightFactor < 0.1) return float3(0, 0, 0);

    // Only draw stars in upper hemisphere
    if (viewDir.y < 0.0) return float3(0, 0, 0);

    // Convert view direction to spherical coordinates for texture sampling
    float2 starUV;
    starUV.x = atan2(viewDir.x, viewDir.z) / (2.0 * PI) + 0.5;
    starUV.y = asin(viewDir.y) / PI + 0.5;

    float stars = starTexture.Sample(linearSampler, starUV).r;

    // Make stars twinkle
    stars *= (sin(timeOfDay * 100.0 + viewDir.x * 50.0) * 0.5 + 0.5) * 0.3 + 0.7;

    return float3(stars, stars, stars * 0.9) * starIntensity * nightFactor;
}

// PIXEL SHADER MAIN
float4 main(PixelInput input) : SV_TARGET
{
    float3 viewDir = normalize(input.viewDirection);

    // DAY/NIGHT CYCLE
    float dayNightBlend = GetDayNightBlend(timeOfDay);
    float nightFactor = 1.0 - dayNightBlend;

    // BASE SKY GRADIENT - IMPROVED
    float horizonFactor = abs(viewDir.y); // 0 at horizon, 1 at zenith
    horizonFactor = pow(horizonFactor, 0.7); // Gentler than before
    horizonFactor = smoothstep(0.0, 1.0, horizonFactor);

    // BETTER DAY COLORS
    float3 dayZenith = float3(0.1, 0.3, 0.9);      // Deep blue at top
    float3 dayMid = float3(0.4, 0.6, 0.95);        // Mid blue
    float3 dayHorizon = float3(0.7, 0.85, 1.0);    // Light blue at horizon

    float3 daySky;
    if (horizonFactor > 0.5)
    {
        // Upper half: zenith to mid
        float t = (horizonFactor - 0.5) * 2.0;
        daySky = lerp(dayMid, dayZenith, t);
    }
    else
    {
        // Lower half: horizon to mid
        float t = horizonFactor * 2.0;
        daySky = lerp(dayHorizon, dayMid, t);
    }

    // BETTER NIGHT COLORS - NOT PURE BLACK!
    float3 nightZenith = float3(0.01, 0.01, 0.05);
    float3 nightMid = float3(0.03, 0.03, 0.08);
    float3 nightHorizon = float3(0.05, 0.05, 0.12);

    float3 nightSky;
    if (horizonFactor > 0.5)
    {
        float t = (horizonFactor - 0.5) * 2.0;
        nightSky = lerp(nightMid, nightZenith, t);
    }
    else
    {
        float t = horizonFactor * 2.0;
        nightSky = lerp(nightHorizon, nightMid, t);
    }

    // Smooth blend between day and night
    float3 skyColor = lerp(nightSky, daySky, dayNightBlend);

    // ATMOSPHERIC SCATTERING (only during day)
    if (dayNightBlend > 0.1)
    {
        float sunDot = dot(viewDir, sunDirection);

        // Rayleigh scattering (blue sky)
        float rayleigh = 0.05 * (1.0 + sunDot * sunDot);

        // Add subtle scattering
        float3 scatterColor = float3(0.3, 0.5, 0.8);
        skyColor += scatterColor * rayleigh * (1.0 - horizonFactor) * dayNightBlend;
    }

    // SUN (only visible during day)
    if (dayNightBlend > 0.1)
    {
        float sun = DrawCelestialBody(viewDir, sunDirection, sunSize);
        skyColor += sunColor * sun * sunIntensity * dayNightBlend;

        // Sun glow
        float sunGlow = exp(-distance(viewDir, sunDirection) * 2.0) * 0.3;
        skyColor += sunColor * sunGlow * sunIntensity * dayNightBlend;
    }

    // MOON (only visible at night)
    if (nightFactor > 0.1)
    {
        float moon = DrawCelestialBody(viewDir, moonDirection, moonSize);

        // Simple moon rendering without texture dependency
        // Add slight variation across the moon surface for visual interest
        float3 moonUp = float3(0, 1, 0);
        if (abs(dot(moonDirection, moonUp)) > 0.95)
        {
            moonUp = float3(1, 0, 0);
        }
        float3 moonRight = normalize(cross(moonUp, moonDirection));
        moonUp = normalize(cross(moonDirection, moonRight));

        // Create subtle surface detail procedurally
        float2 moonUV;
        moonUV.x = dot(viewDir, moonRight);
        moonUV.y = dot(viewDir, moonUp);

        // Simple procedural "craters" - darken some areas slightly
        float detail = 1.0;
        if (moon > 0.1)
        {
            float noise = frac(sin(dot(moonUV * 5.0, float2(12.9898, 78.233))) * 43758.5453);
            detail = lerp(0.9, 1.0, noise);  // Less darkening
        }

        // Bright white moon color
        float3 brightMoonColor = float3(1.0, 1.0, 0.95);  // Pure white, slight warm tint

        // Moon disk - bright and solid
        float3 moonDisk = brightMoonColor * moon * detail * 1.5;

        // Subtle glow - tight around the moon only
        float moonGlow = exp(-distance(viewDir, moonDirection) * 6.0) * 0.25;
        float3 moonGlowColor = brightMoonColor * moonGlow;

        skyColor += (moonDisk + moonGlowColor) * nightFactor;
    }

    // STARS (only at night)
    float3 stars = DrawStars(viewDir, nightFactor);
    skyColor += stars;

    // SUNRISE/SUNSET GLOW - FIXED
    float sunsetFactor = 0.0;

    // Sunrise (0.2 - 0.3)
    if (timeOfDay > 0.2 && timeOfDay < 0.3)
    {
        sunsetFactor = smoothstep(0.2, 0.25, timeOfDay) * smoothstep(0.3, 0.25, timeOfDay);
    }
    // Sunset (0.7 - 0.8)
    else if (timeOfDay > 0.7 && timeOfDay < 0.8)
    {
        sunsetFactor = smoothstep(0.7, 0.75, timeOfDay) * smoothstep(0.8, 0.75, timeOfDay);
    }

    // Only show sunset colors near horizon
    if (sunsetFactor > 0.0)
    {
        float horizonBand = 1.0 - abs(viewDir.y);
        horizonBand = pow(horizonBand, 2.0);

        // Sunset color gradient
        float3 sunsetOrange = float3(1.0, 0.6, 0.2);
        float3 sunsetPink = float3(1.0, 0.4, 0.5);
        float3 sunsetColor = lerp(sunsetOrange, sunsetPink, viewDir.y * 0.5 + 0.5);

        skyColor += sunsetColor * horizonBand * sunsetFactor * 0.6;
    }

    float horizonHaze = pow(1.0 - abs(viewDir.y), 3.0);
    float3 hazeColor = lerp(
        float3(0.05, 0.05, 0.1),   // Night haze
        float3(0.7, 0.8, 0.9),     // Day haze
        dayNightBlend
    );
    skyColor = lerp(skyColor, hazeColor, horizonHaze * 0.2);
    return float4(skyColor, 1.0);
}