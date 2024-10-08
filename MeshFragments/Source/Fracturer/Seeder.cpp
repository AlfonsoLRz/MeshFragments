#include "stdafx.h"
#include "Seeder.h"

#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>

namespace fracturer
{
    Halton_sampler  Seeder::_haltonSampler;
    Halton_enum     Seeder::_haltonEnum (1, 1);

    boost::mt19937  Seeder::_boostRng;
    boost::normal_distribution<> Seeder::_boostNormalDistribution = boost::normal_distribution<>(.5, .25);
    std::unique_ptr<boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>> Seeder::_boostGenerator;

    fracturer::Seeder::RandomInitUniformMap Seeder::_randomInitFunction = {
        { FractureParameters::STD_UNIFORM, [](int size) -> void {}},
        { FractureParameters::HALTON, [](int size) -> void { Seeder::_haltonSampler.init_faure(); Seeder::_haltonEnum = Halton_enum(size, 1); }},	
        { FractureParameters::BOOST_NORMAL_DISTRIBUTION, [](int size) -> void { 
            Seeder::_boostGenerator.reset(new boost::variate_generator<boost::mt19937&, boost::normal_distribution<>>(
                Seeder::_boostRng, Seeder::_boostNormalDistribution
            ));
        }}
    };

    fracturer::Seeder::RandomUniformMap Seeder::_randomFunction = {
        { FractureParameters::STD_UNIFORM, [](int min, int max, int index, int coord) -> int { return RandomUtilities::getUniformRandomInt(min, max); }},
        { FractureParameters::HALTON, [](int min, int max, int index, int coord) -> int { return int(Seeder::_haltonSampler.sample(coord, index) * (max - min) + min); }},
        { FractureParameters::BOOST_NORMAL_DISTRIBUTION, [](int min, int max, int index, int coord) -> int { return int(glm::clamp((*Seeder::_boostGenerator.get())(), .0, 1.0) * (max - min) + min); }}
    };

    fracturer::Seeder::RandomUniformMapFloat Seeder::_randomFunctionFloat = {
        { FractureParameters::STD_UNIFORM, [](float min, float max, int index, int coord) -> float { return RandomUtilities::getUniformRandom(min, max); }},
        { FractureParameters::HALTON, [](float min, float max, int index, int coord) -> float { return Seeder::_haltonSampler.sample(coord, index) * (max - min) + min; }},
        { FractureParameters::BOOST_NORMAL_DISTRIBUTION, [](float min, float max, int index, int coord) -> float { return glm::clamp((*Seeder::_boostGenerator.get())(), .0, 1.0) * (max - min) + min; }}
    };

    void Seeder::getFloatNoise(unsigned int maxBufferSize, unsigned int nseeds, int randomSeedFunction, std::vector<float>& noiseBuffer)
    {
        _randomInitFunction[randomSeedFunction](maxBufferSize);

        for (unsigned int seed = 0; seed < nseeds; seed += 2)
        {
            noiseBuffer.push_back(_randomFunctionFloat[randomSeedFunction](.0f, 1.0f, seed / 2, 0));
            noiseBuffer.push_back(_randomFunctionFloat[randomSeedFunction](.0f, 1.0f, seed / 2, 1));
        }
    }

    std::vector<glm::uvec4> Seeder::nearSeeds(const RegularGrid& grid, const std::vector<glm::uvec4>& frags, unsigned numImpacts, unsigned numSeeds, unsigned spreading)
	{
        // Custom glm::uvec3 comparator
        auto comparator = [](const glm::uvec3& lhs, const glm::uvec3& rhs) {
            if (lhs.x != rhs.x) return lhs.x < rhs.x;
            else if (lhs.y != rhs.y) return lhs.y < rhs.y;
            else                     return lhs.z < rhs.z;
        };

        // Set where to store seeds
        std::set<glm::uvec3, decltype(comparator)> seeds(comparator);
        unsigned currentSeeds, nseeds, numPendingSeeds = numSeeds;
        uvec3 numDivs = grid.getNumSubdivisions(), numDivs2 = numDivs / uvec3(2);
        unsigned minDiv = glm::min(numDivs.x, glm::min(numDivs.y, numDivs.z)) / 2;

		for (int idx = 0; idx < numImpacts; ++idx)
		{
            uvec4 frag = frags[RandomUtilities::getUniformRandomInt(0, frags.size() - 1)];
            nseeds = RandomUtilities::getUniformRandomInt(1, numPendingSeeds);
            currentSeeds = 0;
			
            // Bruteforce seed search
            while (currentSeeds != nseeds) 
            {
                // Generate random seed
                int x = numDivs2.x - RandomUtilities::getBiasedRandomInt(0, numDivs.x, spreading);
                int y = numDivs2.y - RandomUtilities::getBiasedRandomInt(0, numDivs.y, spreading);
                int z = numDivs2.z - RandomUtilities::getBiasedRandomInt(0, numDivs.z, spreading);

                x = (frag.x + x + numDivs.x) % numDivs.x;
                y = (frag.y + y + numDivs.y) % numDivs.y;
                z = (frag.z + z + numDivs.z) % numDivs.z;
            	
                glm::uvec3 voxel(x, y, z);
                if (glm::distance(vec3(frag), vec3(voxel)) > minDiv) continue; // Skip if too far from fragment

                // Is occupied the voxel?
                bool occupied = grid.isOccupied(x, y, z);

                // Is repeated?
                bool isFree = seeds.find(voxel) == seeds.end();

                // Is on the surface?
                bool isBoundary = grid.isBoundary(x, y, z);

                if (occupied && isFree && isBoundary)
                {
                    seeds.insert(voxel);
                    ++currentSeeds;
                }
            }

            numPendingSeeds -= nseeds;
		}

        // Array of generated seeds
        std::vector<glm::uvec4> result = frags;

        // Generate array of seed
        unsigned int nseed = result.empty() ? VOXEL_FREE: result.back().w;
        for (glm::uvec3 seed : seeds)
            result.push_back(glm::uvec4(seed, ++nseed));

        return result;
	}
	
    void Seeder::mergeSeeds(const std::vector<glm::uvec4>& frags, std::vector<glm::uvec4>& seeds, DistanceFunction dfunc) 
    {
        std::vector<int> idFragment(std::pow(2, VOXEL_ID_POSITION), 0);

        for (auto& seed : seeds) 
        {
            float min   = std::numeric_limits<float>::max();
            int nearest = -1;
        	
            for (unsigned int i = 0; i < frags.size(); ++i) 
            {
                glm::vec3 f = frags[i];
                glm::vec3 s = glm::vec3(seed);
                float distance = .0f;

                switch (dfunc) {
                case EUCLIDEAN_DISTANCE:
                    distance = glm::distance(s, f);
                    break;
                case MANHATTAN_DISTANCE:
                    distance = abs(s.x - f.x) + abs(s.y - f.y) + abs(s.z - f.z);
                    break;
                case CHEBYSHEV_DISTANCE:
                    distance = glm::max(abs(s.x - f.x), glm::max(abs(s.y - f.y), abs(s.z - f.z)));
                    break;
                }

                if (distance < min) 
                {
                    min = distance;
                    nearest = i;
                }
            }

            // Nearest found?
            seed.w = frags[nearest].w | (++idFragment[frags[nearest].w] << Seeder::VOXEL_ID_POSITION);
        }
    }

    std::vector<glm::uvec4> Seeder::uniform(const RegularGrid& grid, unsigned int nseeds, int randomSeedFunction, Location location) {
        // Custom glm::uvec3 comparator
        auto comparator = [](const glm::uvec3& lhs, const glm::uvec3& rhs) {
            if      (lhs.x != rhs.x) return lhs.x < rhs.x;
            else if (lhs.y != rhs.y) return lhs.y < rhs.y;
            else                     return lhs.z < rhs.z;
        };

        // Set where to store seeds
        std::set<glm::uvec3, decltype(comparator)> seeds(comparator);
        // Current try number
        uvec3 numDivs = grid.getNumSubdivisions() - uvec3(2);
        unsigned int attempt = 0, maxDim = glm::max(numDivs.x, glm::max(numDivs.y, numDivs.z));

        _randomInitFunction[randomSeedFunction](maxDim);

        // Bruteforce seed search
        while (seeds.size() != nseeds) {
            // Check attempt number
            if (attempt == MAX_TRIES)
                throw SeederSearchError("Max. number of tries surpassed (" + std::to_string(MAX_TRIES) + ")");

            // Generate random seed
            int x = _randomFunction[randomSeedFunction](0, numDivs.x + 1, attempt, 0);
            int y = _randomFunction[randomSeedFunction](0, numDivs.y + 1, attempt, 1);
            int z = _randomFunction[randomSeedFunction](0, numDivs.z + 1, attempt, 2);
            glm::uvec3 voxel(x, y, z);

            // Is occupied the voxel?
            bool occupied = grid.isOccupied(x, y, z);

            // Is on the mesh surface?
            bool isBoundary = grid.isBoundary(x, y, z);

            // Is repeated?
            bool isFree = seeds.find(voxel) == seeds.end();

            if (occupied && isFree)
                if ((location == OUTER && isBoundary) || (location == INNER && !isBoundary) || location == BOTH)
                    seeds.insert(voxel);

            attempt++;
        }

        // Array of generated seeds
        std::vector<glm::uvec4> result;

        // Generate array of seed
        unsigned int nseed = VOXEL_FREE + 1;         // 2 because first seed id must be greater than 1
    	
        for (glm::uvec3 seed : seeds)
            result.push_back(glm::uvec4(seed, nseed++));

        return result;
    }
}
