
#include <mlopen.h>
#include "test.hpp"
#include <array>
#include <iterator>
#include <memory>
#include <utility>
#include <iostream>
#include <mlopen/tensor.hpp>
#include <mlopen/convolution.hpp>
#include <mlopen/softmax.hpp>
#include <limits>

#include "tensor_holder.hpp"
#include "verify.hpp"
#include "driver.hpp"
#include "get_handle.hpp"


struct verify_forward_sofmax
{
    template<class T>
    tensor<T> cpu(const tensor<T>& input)
    {
        auto out = input;
        std::fill(out.begin(), out.end(), 0);

        int in_n, in_c, in_h, in_w;
        std::tie(in_n, in_c, in_h, in_w) = mlopen::tie4(input.desc.GetLengths());

        par_ford(in_n, in_h, in_w)([&](int o, int i, int j)
        {
            T max_c = std::numeric_limits<T>::lowest();
            ford(in_c)([&](int w)
            {
                max_c = std::max(max_c, input(o, w, i, j));
            });

            T sum = 0;
            ford(in_c)([&](int w)
            {
                sum += exp(input(o, w, i, j) - max_c);
            });

            ford(in_c)([&](int w)
            {
                out(o, w, i, j) = exp(input(o, w, i, j) - max_c) / sum;
            });

        });
        return out;
    }

    template<class T>
    tensor<T> gpu(const tensor<T>& input)
    {
        auto&& handle = get_handle();
        auto out = input;

        auto out_dev = handle.Write(out.data);

        int alpha = 1, beta = 1;

        mlopen::SoftmaxForward(handle, &alpha, &beta, input.desc, out_dev.get());

        out.data = handle.Read<T>(out_dev, out.data.size());
        return out;
    }

    template<class T>
    void fail(float, const tensor<T>& input)
    {
        std::cout << "Forward Sofmax: " << std::endl;
        std::cout << "Input tensor: " << input.desc.ToString() << std::endl;
    }
};

struct verify_backward_sofmax
{
    template<class T>
    tensor<T> cpu(const tensor<T>& out, const tensor<T>& dout)
    {
        auto input = dout;

        int in_n, in_c, in_h, in_w;
        std::tie(in_n, in_c, in_h, in_w) = mlopen::tie4(input.desc.GetLengths());

        par_ford(in_n, in_h, in_w)([&](int o, int i, int j)
        {
            T sum = 0;
            ford(in_c)([&](int c)
            {
                sum += out(o, c, i, j) * dout(o, c, i, j);
            });

            ford(in_c)([&](int c)
            {
                input(o, c, i, j) = out(o, c, i, j) * (input(o, c, i, j) - sum);
            });
        });
        return input;
    }

    template<class T>
    tensor<T> gpu(const tensor<T>& out, const tensor<T>& dout)
    {
        auto&& handle = get_handle();
        auto input = dout;

        auto in_dev = handle.Write(input.data);
        auto out_dev = handle.Write(out.data);

        int alpha = 1, beta = 1;

        mlopen::SoftmaxBackward(handle, &alpha, out.desc, out_dev.get(), &beta, input.desc, in_dev.get());

        input.data = handle.Read<T>(in_dev, input.data.size());
        return input;
    }

    template<class T>
    void fail(float, const tensor<T>& output, const tensor<T>&)
    {
        std::cout << "Backward Sofmax: " << std::endl;
        std::cout << "Output tensor: " << output.desc.ToString() << std::endl;
    }
};

template<class T>
struct softmax_driver : test_driver
{
    tensor<T> input;

    softmax_driver()
    {
        add(input, "input", generate_tensor());
    }

    void run()
    {
        auto out = verify(verify_forward_sofmax{}, input);
        auto dout = input;
        dout.generate([&](int n, int c, int h, int w)
        {
            T x = input(n, c, h, w);
            double y = (877*n+547*c+701*h+1049*w+static_cast<int>(769*x))%2503;
            return ((x*y)/1301.0);
        });
        verify(verify_backward_sofmax{}, out.first, dout);
    }
};

int main(int argc, const char *argv[]) 
{
    test_drive<softmax_driver<float>>(argc, argv);
}

