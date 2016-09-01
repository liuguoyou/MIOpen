/* ************************************************************************
 * Copyright 2015 Vratis, Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ************************************************************************ */


#include <mlopen/kernel_cache.hpp>

#include <iostream>
#include <iterator>

namespace mlopen {

OCLKernel KernelCache::GetKernel(const std::string& algorithm,
										const std::string& network_config) {

	std::pair<std::string, std::string> key = std::make_pair(algorithm, network_config);
#ifndef NDEBUG
	std::cout << "key: " << key.first <<" "<< key.second<< std::endl;
#endif

	auto kernel_iterator = kernel_map.find(key);
	if (kernel_iterator != kernel_map.end())
	{
#ifndef NDEBUG
		printf("kernel found\n");
#endif
		return kernel_iterator->second;
	}
	else
	{
        MLOPEN_THROW("looking for default kernel (does not exist): " + algorithm + ", " + network_config);
	}
}

OCLKernel KernelCache::GetKernel(cl_command_queue &queue,
										const std::string& algorithm,
										const std::string& network_config,
                                        const std::string& program_name,
                                        const std::string& kernel_name,
										const std::vector<size_t>& vld,
										const std::vector<size_t>& vgd,
                                        std::string params)
{

    if (params.length() > 0)
    {
        // Ensure only one space after the -cl-std.
        // >1 space can cause an Apple compiler bug. See clSPARSE issue #141.
        if (params.at(0) != ' ') { params = " " + params; }
    }

	std::pair<std::string, std::string> key = std::make_pair(algorithm, network_config);
#ifndef NDEBUG
    std::cout << "key: " << key.first << ',' << key.second << std::endl;
#endif

    auto kernel_iterator = kernel_map.find(key);
    if (kernel_iterator != kernel_map.end())
    {
#ifndef NDEBUG
		printf("kernel found\n");
#endif
        return kernel_iterator->second;
    }
    else //build program and compile the kernel;
    {
#ifndef NDEBUG
		printf("kernel not found\n");
#endif
	
        auto program = LoadProgram(GetContext(queue), GetDevice(queue), program_name, params);	
		OCLKernel kernel(CreateKernel(program, kernel_name), vld, vgd);
        if (!network_config.empty() && !algorithm.empty()) { kernel_map[key] = kernel; }
        return kernel;
    }
}

KernelCache::KernelCache()
{}

} // namespace mlopen