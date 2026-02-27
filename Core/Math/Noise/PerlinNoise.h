//
// Created by Marvin on 30/01/2026.
//

#ifndef VOXTAU_PERLINNOISE_H
#define VOXTAU_PERLINNOISE_H

#include <EngineApi.h>
#include <vector>
#include <cstdint>

class PerlinNoise
{
private:
    std::vector<int> m_permutation;

    // Fade function for smooth interpolation: 6t^5 - 15t^4 + 10t^3
    static double Fade(double t);

    // Linear interpolation
    static double Lerp(double t, double a, double b);

    // Gradient function - computes dot product of gradient and distance vectors
    static double Grad(int hash, double x, double y, double z);

public:
    // Default constructor uses a fixed seed for reproducibility
    PerlinNoise();

    // Constructor with custom seed
    explicit PerlinNoise(uint32_t seed);

    // Reseed the noise generator
    void Reseed(uint32_t seed);

    // Get noise value at 3D coordinates (returns value in range [-1, 1])
    double Noise(double x, double y, double z) const;

    // Get noise value at 2D coordinates (returns value in range [-1, 1])
    double Noise(double x, double y) const;

    // Get noise value normalized to [0, 1]
    double Noise01(double x, double y) const;
    double Noise01(double x, double y, double z) const;

    // Fractal/Octave noise for more natural-looking terrain
    // - octaves: number of noise layers
    // - persistence: amplitude multiplier per octave (typically 0.5)
    // - lacunarity: frequency multiplier per octave (typically 2.0)
    double OctaveNoise(double x, double y, int octaves, double persistence = 0.5, double lacunarity = 2.0) const;
    double OctaveNoise(double x, double y, double z, int octaves, double persistence = 0.5, double lacunarity = 2.0) const;

    // Octave noise normalized to [0, 1]
    double OctaveNoise01(double x, double y, int octaves, double persistence = 0.5, double lacunarity = 2.0) const;
};


#endif //VOXTAU_PERLINNOISE_H