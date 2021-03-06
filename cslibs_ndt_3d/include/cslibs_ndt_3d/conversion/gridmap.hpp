#ifndef CSLIBS_NDT_3D_CONVERSION_GRIDMAP_HPP
#define CSLIBS_NDT_3D_CONVERSION_GRIDMAP_HPP

#include <cslibs_ndt_3d/dynamic_maps/gridmap.hpp>
#include <cslibs_ndt_3d/static_maps/gridmap.hpp>

namespace cslibs_ndt_3d {
namespace conversion {
inline cslibs_ndt_3d::dynamic_maps::Gridmap::Ptr from(
        const cslibs_ndt_3d::static_maps::Gridmap::Ptr& src)
{
    if (!src)
        return nullptr;

    using src_map_t = cslibs_ndt_3d::static_maps::Gridmap;
    using dst_map_t = cslibs_ndt_3d::dynamic_maps::Gridmap;
    typename dst_map_t::Ptr dst(new dst_map_t(src->getInitialOrigin(),
                                              src->getResolution()));

    using index_t = std::array<int, 3>;
    src->traverse([&dst](const index_t &bi, const typename src_map_t::distribution_bundle_t &b){
        if (const typename dst_map_t::distribution_bundle_t* b_dst = dst->getDistributionBundle(bi)) {
            for (std::size_t i = 0 ; i < 8 ; ++i)
                b_dst->at(i)->data() = b.at(i)->data();
        }
    });

    return dst;
}

inline cslibs_ndt_3d::static_maps::Gridmap::Ptr from(
        const cslibs_ndt_3d::dynamic_maps::Gridmap::Ptr& src)
{
    if (!src)
        return nullptr;

    using index_t = std::array<int, 3>;
    const index_t min_distribution_index =
            cslibs_math::common::cast<int>(std::floor(cslibs_math::common::cast<double>(src->getMinBundleIndex()) / 2.0) * 2.0);
    const index_t max_distribution_index =
            cslibs_math::common::cast<int>( std::ceil(cslibs_math::common::cast<double>(src->getMaxBundleIndex()) / 2.0) * 2.0 + 1.0);

    const std::array<std::size_t, 3> size =
            cslibs_math::common::cast<std::size_t>(std::ceil(cslibs_math::common::cast<double>(max_distribution_index - min_distribution_index) / 2.0));

    using src_map_t = cslibs_ndt_3d::dynamic_maps::Gridmap;
    using dst_map_t = cslibs_ndt_3d::static_maps::Gridmap;
    typename dst_map_t::Ptr dst(new dst_map_t(src->getInitialOrigin(),
                                              src->getResolution(),
                                              size,
                                              min_distribution_index));

    src->traverse([&dst](const index_t &bi, const typename src_map_t::distribution_bundle_t &b){
        if (const typename dst_map_t::distribution_bundle_t* b_dst = dst->getDistributionBundle(bi)) {
            for (std::size_t i = 0 ; i < 8 ; ++i)
                b_dst->at(i)->data() = b.at(i)->data();
        }
    });

    return dst;
}
}
}

#endif // CSLIBS_NDT_3D_CONVERSION_GRIDMAP_HPP
