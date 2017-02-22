#include <algorithm>
#include <vector>

#include "caffe/layer.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/vision_layers.hpp"

namespace caffe {

template <typename Dtype, typename Mtype>
void PowerLayer<Dtype,Mtype>::LayerSetUp(const vector<Blob<Dtype,Mtype>*>& bottom,
      const vector<Blob<Dtype,Mtype>*>& top) {
  NeuronLayer<Dtype,Mtype>::LayerSetUp(bottom, top);
  power_ = this->layer_param_.power_param().power();
  scale_ = this->layer_param_.power_param().scale();
  shift_ = this->layer_param_.power_param().shift();
  diff_scale_ = power_  * scale_;
}

// Compute y = (shift + scale * x)^power
template <typename Dtype, typename Mtype>
void PowerLayer<Dtype,Mtype>::Forward_cpu(const vector<Blob<Dtype,Mtype>*>& bottom,
    const vector<Blob<Dtype,Mtype>*>& top) {
  Dtype* top_data = top[0]->mutable_cpu_data();
  const int count = bottom[0]->count();
  // Special case where we can ignore the input: scale or power is 0.
  if (diff_scale_ == Mtype(0)) {
    Mtype value(power_ == 0 ? Mtype(1.) : Mtype(pow(shift_, power_)));
    caffe_set(count, Get<Dtype>(value), top_data);
    return;
  }
  const Dtype* bottom_data = bottom[0]->cpu_data();
  caffe_copy<Dtype,Mtype>(count, bottom_data, top_data);
  if (scale_ != Mtype(1)) {
    caffe_scal<Dtype,Mtype>(count, scale_, top_data);
  }
  if (shift_ != Mtype(0)) {
    caffe_add_scalar<Dtype,Mtype>(count, shift_, top_data);
  }
  if (power_ != Mtype(1)) {
    caffe_powx<Dtype,Mtype>(count, top_data, power_, top_data);
  }
}

template <typename Dtype, typename Mtype>
void PowerLayer<Dtype,Mtype>::Backward_cpu(const vector<Blob<Dtype,Mtype>*>& top,
    const vector<bool>& propagate_down,
    const vector<Blob<Dtype,Mtype>*>& bottom) {
  if (propagate_down[0]) {
    Dtype* bottom_diff = bottom[0]->mutable_cpu_diff();
    const int count = bottom[0]->count();
    const Dtype* top_diff = top[0]->cpu_diff();
    if (diff_scale_ == Mtype(0) || power_ == Mtype(1)) {
      caffe_set(count, Get<Dtype>(diff_scale_), bottom_diff);
    } else {
      const Dtype* bottom_data = bottom[0]->cpu_data();
      // Compute dy/dx = scale * power * (shift + scale * x)^(power - 1)
      //               = diff_scale * y / (shift + scale * x)
      if (power_ == Mtype(2)) {
        // Special case for y = (shift + scale * x)^2
        //     -> dy/dx = 2 * scale * (shift + scale * x)
        //              = diff_scale * shift + diff_scale * scale * x
        caffe_cpu_axpby<Dtype,Mtype>(count, diff_scale_ * scale_, bottom_data,
            Mtype(0), bottom_diff);
        if (shift_ != Mtype(0)) {
          caffe_add_scalar<Dtype,Mtype>(count, diff_scale_ * shift_, bottom_diff);
        }
      } else if (shift_ == Mtype(0)) {
        // Special case for y = (scale * x)^power
        //     -> dy/dx = scale * power * (scale * x)^(power - 1)
        //              = scale * power * (scale * x)^power * (scale * x)^(-1)
        //              = power * y / x
        const Dtype* top_data = top[0]->cpu_data();
        caffe_div<Dtype,Mtype>(count, top_data, bottom_data, bottom_diff);
        caffe_scal<Dtype,Mtype>(count, power_, bottom_diff);
      } else {
        caffe_copy<Dtype,Mtype>(count, bottom_data, bottom_diff);
        if (scale_ != Mtype(1)) {
          caffe_scal<Dtype,Mtype>(count, scale_, bottom_diff);
        }
        if (shift_ != Mtype(0)) {
          caffe_add_scalar<Dtype,Mtype>(count, shift_, bottom_diff);
        }
        const Dtype* top_data = top[0]->cpu_data();
        caffe_div<Dtype,Mtype>(count, top_data, bottom_diff, bottom_diff);
        if (diff_scale_ != Mtype(1)) {
          caffe_scal<Dtype,Mtype>(count, diff_scale_, bottom_diff);
        }
      }
    }
    if (diff_scale_ != Mtype(0)) {
      caffe_mul<Dtype,Mtype>(count, top_diff, bottom_diff, bottom_diff);
    }
  }
}

#ifdef CPU_ONLY
STUB_GPU(PowerLayer);
#endif

INSTANTIATE_CLASS(PowerLayer);
REGISTER_LAYER_CLASS(Power);

}  // namespace caffe
