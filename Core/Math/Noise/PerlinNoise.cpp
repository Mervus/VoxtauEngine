//
// Created by Marvin on 30/01/2026.
//

#include "PerlinNoise.h"
#include <algorithm>
#include <numeric>
#include <random>

PerlinNoise::PerlinNoise()
{
    // Use default seed
    Reseed(0);
}

PerlinNoise::PerlinNoise(uint32_t seed)
{
    Reseed(seed);
}

void PerlinNoise::Reseed(uint32_t seed)
{
    m_permutation.resize(256);

    // Fill with values 0-255
    std::iota(m_permutation.begin(), m_permutation.end(), 0);

    // Shuffle using the seed
    std::default_random_engine engine(seed);
    std::shuffle(m_permutation.begin(), m_permutation.end(), engine);

    // Duplicate the permutation table to avoid overflow
    m_permutation.insert(m_permutation.end(), m_permutation.begin(), m_permutation.end());
}

double PerlinNoise::Fade(double t)
{
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::Lerp(double t, double a, double b)
{
    return a + t * (b - a);
}

double PerlinNoise::Grad(int hash, double x, double y, double z)
{
    // Convert low 4 bits of hash code into 12 gradient directions
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double PerlinNoise::Noise(double x, double y, double z) const
{
    // Find unit cube that contains the point
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;

    // Find relative x, y, z of point in cube
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    // Compute fade curves for x, y, z
    double u = Fade(x);
    double v = Fade(y);
    double w = Fade(z);

    // Hash coordinates of the 8 cube corners
    int A  = m_permutation[X] + Y;
    int AA = m_permutation[A] + Z;
    int AB = m_permutation[A + 1] + Z;
    int B  = m_permutation[X + 1] + Y;
    int BA = m_permutation[B] + Z;
    int BB = m_permutation[B + 1] + Z;

    // Blend results from 8 corners of cube
    double res = Lerp(w,
        Lerp(v,
            Lerp(u, Grad(m_permutation[AA], x, y, z),
                    Grad(m_permutation[BA], x - 1, y, z)),
            Lerp(u, Grad(m_permutation[AB], x, y - 1, z),
                    Grad(m_permutation[BB], x - 1, y - 1, z))),
        Lerp(v,
            Lerp(u, Grad(m_permutation[AA + 1], x, y, z - 1),
                    Grad(m_permutation[BA + 1], x - 1, y, z - 1)),
            Lerp(u, Grad(m_permutation[AB + 1], x, y - 1, z - 1),
                    Grad(m_permutation[BB + 1], x - 1, y - 1, z - 1)))
    );

    return res;
}

double PerlinNoise::Noise(double x, double y) const
{
    // Use z = 0 for 2D noise
    return Noise(x, y, 0.0);
}

double PerlinNoise::Noise01(double x, double y) const
{
    // Map from [-1, 1] to [0, 1]
    return (Noise(x, y) + 1.0) * 0.5;
}

double PerlinNoise::Noise01(double x, double y, double z) const
{
    return (Noise(x, y, z) + 1.0) * 0.5;
}

double PerlinNoise::OctaveNoise(double x, double y, int octaves, double persistence, double lacunarity) const
{
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double maxValue = 0.0;  // For normalization

    for (int i = 0; i < octaves; ++i) {
        total += Noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    // Normalize to [-1, 1]
    return total / maxValue;
}

double PerlinNoise::OctaveNoise(double x, double y, double z, int octaves, double persistence, double lacunarity) const
{
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double maxValue = 0.0;

    for (int i = 0; i < octaves; ++i) {
        total += Noise(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / maxValue;
}

double PerlinNoise::OctaveNoise01(double x, double y, int octaves, double persistence, double lacunarity) const
{
    return (OctaveNoise(x, y, octaves, persistence, lacunarity) + 1.0) * 0.5;
}