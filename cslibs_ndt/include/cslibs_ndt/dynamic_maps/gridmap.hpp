#ifndef CSLIBS_NDT_DYNAMIC_GRIDMAP_HPP
#define CSLIBS_NDT_DYNAMIC_GRIDMAP_HPP

#include <array>
#include <vector>
#include <cmath>
#include <memory>

#include <cslibs_math_2d/linear/pose.hpp>
#include <cslibs_math_2d/linear/point.hpp>

#include <cslibs_ndt/common/distribution_container.hpp>

#include <cslibs_math/common/array.hpp>

#include <cslibs_indexed_storage/storage.hpp>
#include <cslibs_indexed_storage/backend/kdtree/kdtree.hpp>

namespace cis = cslibs_indexed_storage;

namespace cslibs_ndt {
namespace dynamic_maps {
class Gridmap
{
public:
    using Ptr                   = std::shared_ptr<Gridmap>;
    using pose_t                = cslibs_math_2d::Pose2d;
    using index_t               = std::array<int, 2>;
    using mutex_t               = std::mutex;
    using lock_t                = std::unique_lock<mutex_t>;
    using distribution_t        = DistributionContainer<2, true>;
    using storage_t             = cis::Storage<distribution_t, index_t, cis::backend::kdtree::KDTree>;

    Gridmap(const pose_t        &origin,
            const double         resolution) :
        resolution_(resolution),
        resolution_inv_(1.0 / resolution_),
        w_T_m_(origin),
        m_T_w_(w_T_m_.inverse()),
        min_index_{{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()}},
        max_index_{{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()}},
        storage_(new storage_t)
    {
    }

    Gridmap(const double origin_x,
            const double origin_y,
            const double origin_phi,
            const double resolution) :
        resolution_(resolution),
        resolution_inv_(1.0 / resolution_),
        w_T_m_(origin_x, origin_y, origin_phi),
        m_T_w_(w_T_m_.inverse()),
        min_index_{{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()}},
        max_index_{{std::numeric_limits<int>::min(), std::numeric_limits<int>::min()}},
        storage_(new storage_t)
    {
    }

    inline cslibs_math_2d::Point2d getMin() const
    {
        lock_t l(storage_mutex_);
        return cslibs_math_2d::Point2d(min_index_[0] * resolution_,
                                       min_index_[1] * resolution_);
    }

    inline cslibs_math_2d::Point2d getMax() const
    {
        lock_t l(storage_mutex_);
        return cslibs_math_2d::Point2d((max_index_[0] + 1) * resolution_,
                                       (max_index_[1] + 1) * resolution_);
    }

    inline cslibs_math_2d::Pose2d getOrigin() const
    {
        cslibs_math_2d::Transform2d origin = w_T_m_;
        origin.translation() =  getMin();
        return origin;
    }

    inline cslibs_math_2d::Pose2d getInitialOrigin() const
    {
        return w_T_m_;
    }

    inline void add(const cslibs_math_2d::Point2d &point)
    {
        distribution_t *distribution = nullptr;
        {
            lock_t l(storage_mutex_);
            const index_t index = toIndex(point);
            distribution = storage_->get(index);
            if(distribution == nullptr) {
                distribution = &(storage_->insert(index, distribution_t()));
            }
            updateIndices(index);
        }
        distribution->lock();
        distribution->add(point);
        distribution->setTouched();
        distribution->unlock();
    }

    inline double sample(const cslibs_math_2d::Point2d &point) const
    {
        const index_t index                = toIndex(point);
        const distribution_t *distribution = getDistribution(index);
        auto  get = [distribution, &point](){
            distribution->lock();
            double p = distribution->sample(point);
            distribution->unlock();
            return p;
        };
        return distribution != nullptr ? get() : 0.0;
    }


    inline double sampleNonNormalized(const cslibs_math_2d::Point2d &point) const
    {
        const index_t index                = toIndex(point);
        const distribution_t *distribution = getDistribution(index);
        auto  get = [distribution, &point](){
            distribution->lock();
            double p = distribution->sampleNonNormalized(point);
            distribution->unlock();
            return p;
        };
        return distribution != nullptr ? get() : 0.0;
    }

    inline index_t getMinDistributionIndex() const
    {
        lock_t l(storage_mutex_);
        return min_index_;
    }

    inline index_t getMaxDistributionIndex() const
    {
        lock_t l(storage_mutex_);
        return max_index_;
    }

    inline distribution_t const * getDistribution(const index_t &distribution_index) const
    {
        lock_t l(storage_mutex_);
        return storage_->get(distribution_index);
    }

    inline distribution_t* getDistribution(const index_t &distribution_index)
    {
        lock_t l(storage_mutex_);
        return storage_->get(distribution_index);
    }

    inline double getResolution() const
    {
        return resolution_;
    }

    inline double getHeight() const
    {
        return (max_index_[1] - min_index_[1] + 1) * resolution_;
    }

    inline double getWidth() const
    {
        return (max_index_[0] - min_index_[0] + 1) * resolution_;
    }

protected:
    const double                        resolution_;
    const double                        resolution_inv_;
    const cslibs_math_2d::Transform2d   w_T_m_;
    const cslibs_math_2d::Transform2d   m_T_w_;

    mutable index_t                     min_index_;
    mutable index_t                     max_index_;
    mutable mutex_t                     storage_mutex_;
    mutable std::shared_ptr<storage_t>  storage_;

    inline void updateIndices(const index_t &chunk_index) const
    {
        min_index_ = cslibs_math::common::min(min_index_, chunk_index);
        max_index_ = cslibs_math::common::max(max_index_, chunk_index);
    }

    inline index_t toIndex(const cslibs_math_2d::Point2d &p_w) const
    {
        const cslibs_math_2d::Point2d p_m = m_T_w_ * p_w;
        return {{static_cast<int>(std::floor(p_m(0) * resolution_inv_ + 0.5)),
                 static_cast<int>(std::floor(p_m(1) * resolution_inv_ + 0.5))}};
    }

};
}
}



#endif // CSLIBS_GRIDMAPS_GRIDMAP_HPP
