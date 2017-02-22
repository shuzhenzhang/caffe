#ifdef USE_CUDNN
#include <vector>
#include "caffe/filler.hpp"
#include "caffe/layer.hpp"
#include "caffe/util/gpu_memory.hpp"
#include "caffe/util/im2col.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype, typename Mtype>
void CuDNNLCNLayer<Dtype,Mtype>::Forward_gpu(const vector<Blob<Dtype,Mtype>*>& bottom,
    const vector<Blob<Dtype,Mtype>*>& top) {
  const Dtype* bottom_data = bottom[0]->gpu_data();
  Dtype* top_data = top[0]->mutable_gpu_data();

  gpu_memory::allocate(&this->tempData1, this->tempDataSize);
  gpu_memory::allocate(&this->tempData2, this->tempDataSize);

  CUDNN_CHECK(cudnnDivisiveNormalizationForward(
        Caffe::cudnn_handle(), norm_desc_, CUDNN_DIVNORM_PRECOMPUTED_MEANS,
        cudnn::dataType<Dtype>::one,
        bottom_desc_, bottom_data,
        NULL,  // srcMeansData
        this->tempData1, this->tempData2,
        cudnn::dataType<Dtype>::zero,
        top_desc_, top_data) );

  gpu_memory::deallocate(this->tempData1);
  gpu_memory::deallocate(this->tempData2);
}

template <typename Dtype, typename Mtype>
void CuDNNLCNLayer<Dtype,Mtype>::Backward_gpu(const vector<Blob<Dtype,Mtype>*>& top,
    const vector<bool>& propagate_down, const vector<Blob<Dtype,Mtype>*>& bottom) {
  const Dtype* top_diff = top[0]->gpu_diff();
  const Dtype* top_data = top[0]->gpu_data();
  const Dtype* bottom_data = bottom[0]->gpu_data();
  Dtype* bottom_diff = bottom[0]->mutable_gpu_diff();

  gpu_memory::allocate(&this->tempData1, this->tempDataSize);
  gpu_memory::allocate(&this->tempData2, this->tempDataSize);

  CUDNN_CHECK(cudnnDivisiveNormalizationBackward(
        Caffe::cudnn_handle(), norm_desc_, CUDNN_DIVNORM_PRECOMPUTED_MEANS,
        cudnn::dataType<Dtype>::one,
        bottom_desc_, bottom_data,
        NULL, top_diff,  // NULL - srcMeansData
        this->tempData1, this->tempData2,
        cudnn::dataType<Dtype>::zero,
        bottom_desc_, bottom_diff,
        NULL) );

  gpu_memory::deallocate(this->tempData1);
  gpu_memory::deallocate(this->tempData2);
}

INSTANTIATE_LAYER_GPU_FUNCS(CuDNNLCNLayer);

}  // namespace caffe
#endif
