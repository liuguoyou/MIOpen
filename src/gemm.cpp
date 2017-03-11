#include <mlopen/gemm.hpp>

namespace mlopen {

GemmGeometry CreateGemmGeometryConvBwdWeights(
        const TensorDescriptor&     dyDesc,
        const TensorDescriptor&     xDesc,
        const TensorDescriptor&     dwDesc,
        bool                        isDataColMajor,
        std::string                 &network_config)
{
    int in_n, in_c, in_h, in_w;
    std::tie(in_n, in_c, in_h, in_w) = tie4(xDesc.GetLengths());

    int wei_n, wei_h, wei_w;
    std::tie(wei_n, std::ignore, wei_h, wei_w) = tie4(dwDesc.GetLengths());

    int out_h, out_w;
    std::tie(std::ignore, std::ignore, out_h, out_w) = tie4(dyDesc.GetLengths());

    // GEMM
    int N = in_c * wei_h * wei_w; 
    int M = wei_n; 
    int K = out_h * out_w;  
    bool tA = false;
    bool tB = true;
    bool tC = false;
    int lda = K;
    int ldb = K;
    int ldc = N;
    float alpha = 1.0;
    float beta = 1.0;

    // bool isColMajor, bool tA, bool tB, bool tC, lda, ldb, ldc, m, n, k, a_offset, b_offset, c_offset
    TinyGemmGeometry tgg;
    GemmGeometry gg;
    
    if (!isDataColMajor) {
        tgg = TinyGemmGeometry(true, tB, tA, tC, ldb, lda, ldc, N, M, K, 0, 0, 0);

        gg = GemmGeometry{std::array<int, 3>{{N, M, K}},
            std::array<int, 3>{{ldb, lda, ldc}},
            "mlopenConvolutionBwdWeightsAlgoGEMM",
            alpha, beta, tgg};
    }
    else {
        tgg = TinyGemmGeometry(true, tA, tB, tC, lda, ldb, ldc, M, N, K, 0, 0, 0);

        gg = GemmGeometry{std::array<int, 3>{{M, N, K}},
            std::array<int, 3>{{lda, ldb, ldc}},
            "mlopenConvolutionBwdWeightsAlgoGEMM", 
            alpha, beta, tgg};
    }
    network_config = tgg.get_networkconfig_string();
    return gg;
}

GemmGeometry CreateGemmGeometryConvFwd(
        const TensorDescriptor&     xDesc,
        const TensorDescriptor&     wDesc,
        const TensorDescriptor&     yDesc,
        bool                        isDataColMajor,
        std::string                 &network_config)
{
    int in_n, in_c, in_h, in_w;
    std::tie(in_n, in_c, in_h, in_w) = tie4(xDesc.GetLengths());

    int wei_n, wei_h, wei_w;
    std::tie(wei_n, std::ignore, wei_h, wei_w) = tie4(wDesc.GetLengths());

    int out_h, out_w;
    std::tie(std::ignore, std::ignore, out_h, out_w) = tie4(yDesc.GetLengths());

    // GEMM
    int K = in_c * wei_h * wei_w;
    int M = wei_n;
    int N = out_h * out_w;
    float alpha = 1.0;
    float beta = 0.0;
    bool tA = false;
    bool tB = false;
    bool tC = false;
    int lda = K;
    int ldb = N;
    int ldc = N;

    // bool isColMajor, bool tA, bool tB, bool tC, lda, ldb, ldc, m, n, k, a_offset, b_offset, c_offset
    TinyGemmGeometry tgg;
    GemmGeometry gg;
    
    if (!isDataColMajor) {
        tgg = TinyGemmGeometry(true, tB, tA, tC, ldb, lda, ldc, N, M, K, 0, 0, 0);

        gg = GemmGeometry{std::array<int, 3>{{N, M, K}}, 
            std::array<int, 3>{{ldb, lda, ldc}},
            "mlopenConvolutionFwdAlgoGEMM",
            alpha, beta, tgg};
    }
    else {
        tgg = TinyGemmGeometry(false, tA, tB, tC, lda, ldb, ldc, M, N, K, 0, 0, 0);

        gg = GemmGeometry{std::array<int, 3>{{M, N, K}},
            std::array<int, 3>{{lda, ldb, ldc}},
            "mlopenConvolutionFwdAlgoGEMM", 
            alpha, beta, tgg};
    }
    network_config = tgg.get_networkconfig_string();
    return gg;
}

GemmGeometry CreateMLOpenGemmGeometry( 
        int M, int N, int K,
        int lda, int ldb, int ldc,
        bool tA, bool tB,
        bool isDataColMajor,
        float alpha, float beta)
{
    TinyGemmGeometry tgg;
    
    // Assuming we are using tinygemm as only col major
    // Therefore, if the user provides data in col. major
    // then no transformations are requrired and vice versa
    if(isDataColMajor) {        
        tgg = TinyGemmGeometry(true, tA, tB, false, lda, ldb, ldc, M, N, K, 0, 0, 0);

        return GemmGeometry{std::array<int, 3>{{M, N, K}},
            std::array<int, 3>{{lda, ldb, ldc}},
            "mlopenGEMM",
            alpha, beta, tgg};
    }
    else {
        tgg = TinyGemmGeometry(true, tB, tA, false, ldb, lda, ldc, N, M, K, 0, 0, 0);

        return GemmGeometry{std::array<int, 3>{{N, M, K}},
            std::array<int, 3>{{ldb, lda, ldc}}, 
            "mlopenGEMM",
            alpha, beta, tgg};
    }
}

GemmGeometry GetGemmGeometry(std::string algorithm_name, std::string network_config)
{
    auto gemm_iterator = gemm_geo_map.find(std::make_pair(algorithm_name, network_config));
    if (gemm_iterator != gemm_geo_map.end())
    {
        return gemm_iterator->second;
    }
    else
    {
        MLOPEN_THROW("looking for gemm kernel (does not exist): " + algorithm_name + ", " + network_config);
    }
}

} // namespace mlopen