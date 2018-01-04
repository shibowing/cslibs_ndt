#ifndef CSLIBS_NDT_2D_SERIALIZATION_STATIC_MAPS_GRIDMAP_HPP
#define CSLIBS_NDT_2D_SERIALIZATION_STATIC_MAPS_GRIDMAP_HPP

#include <cslibs_ndt/common/serialization/indexed_distribution.hpp>
#include <cslibs_ndt_2d/static_maps/gridmap.hpp>
#include <cslibs_math_2d/serialization/transform.hpp>

#include <eigen3/Eigen/StdVector>
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(cslibs_math::statistics::Distribution<2, 3>)

#include <yaml-cpp/yaml.h>

namespace YAML {
template <>
struct convert<cslibs_ndt_2d::static_maps::Gridmap::Ptr>
{
    static Node encode(const cslibs_ndt_2d::static_maps::Gridmap::Ptr &rhs)
    {
        Node n;
        if (!rhs)
            return n;

        n.push_back(rhs->getOrigin());
        n.push_back(rhs->getResolution());

        const std::array<std::size_t, 2> & size = rhs->getSize();
        n.push_back(size);

        using vector_t = std::vector<cslibs_math::statistics::Distribution<2, 3>>;
        using index_t = std::array<int, 2>;

        auto divx = [](const index_t & bi) { return cslibs_math::common::div<int>(bi[0], 2); };
        auto divy = [](const index_t & bi) { return cslibs_math::common::div<int>(bi[1], 2); };
        auto modx = [](const index_t & bi) { return cslibs_math::common::mod<int>(bi[0], 2); };
        auto mody = [](const index_t & bi) { return cslibs_math::common::mod<int>(bi[1], 2); };

        auto get_storage_index = [&divx, &divy, &modx, &mody](const index_t & bi, const std::size_t i) {
            return index_t({{(i % 2 == 0) ? divx(bi) : (divx(bi) + modx(bi)),
                             (i < 2) ? divy(bi) : (divy(bi) + mody(bi))}});
        };

        auto get_index = [&size, &get_storage_index] (const index_t & bi, const std::size_t i) {
            const index_t & storage_index = get_storage_index(bi, i);
            return (i == 0) ?
                        (storage_index[1] * size[0] + storage_index[0]) :
                        (storage_index[1] * (size[0] + 1) + storage_index[0]);
        };

        for (std::size_t i = 0 ; i < 4 ; ++ i) {
            vector_t storage(i == 0 ? (size[0] * size[1]) : ((size[0] + 1) * (size[1] + 1)));

            for (int idx = 0 ; idx < static_cast<int>(rhs->getBundleSize()[0]) ; ++ idx) {
                for (int idy = 0 ; idy < static_cast<int>(rhs->getBundleSize()[1]) ; ++ idy) {
                    const index_t bi({idx, idy});

                    if (const typename cslibs_ndt_2d::static_maps::Gridmap::distribution_bundle_t* b =
                            rhs->getDistributionBundle(bi))
                        storage[get_index(bi, i)] = b->at(i)->data();
                }
            }

            n.push_back(storage);
        }

        return n;
    }

    static bool decode(const Node& n, cslibs_ndt_2d::static_maps::Gridmap::Ptr &rhs)
    {
        if (!n.IsSequence() || n.size() != 7)
            return false;

        rhs.reset(new cslibs_ndt_2d::static_maps::Gridmap(
                      n[0].as<cslibs_math_2d::Transform2d>(), n[1].as<double>(), n[2].as<std::array<std::size_t, 2>>()));


        const std::array<std::size_t, 2> & size = rhs->getSize();

        using vector_t = std::vector<cslibs_math::statistics::Distribution<2, 3>>;
        using index_t = std::array<int, 2>;

        auto divx = [](const index_t & bi) { return cslibs_math::common::div<int>(bi[0], 2); };
        auto divy = [](const index_t & bi) { return cslibs_math::common::div<int>(bi[1], 2); };
        auto modx = [](const index_t & bi) { return cslibs_math::common::mod<int>(bi[0], 2); };
        auto mody = [](const index_t & bi) { return cslibs_math::common::mod<int>(bi[1], 2); };

        auto get_storage_index = [&divx, &divy, &modx, &mody](const index_t & bi, const std::size_t i) {
            return index_t({{(i % 2 == 0) ? divx(bi) : (divx(bi) + modx(bi)),
                             (i < 2) ? divy(bi) : (divy(bi) + mody(bi))}});
        };

        auto get_index = [&size, &get_storage_index] (const index_t & bi, const std::size_t i) {
            const index_t & storage_index = get_storage_index(bi, i);
            return (i == 0) ?
                        (storage_index[1] * size[0] + storage_index[0]) :
                        (storage_index[1] * (size[0] + 1) + storage_index[0]);
        };

        for (std::size_t i = 0 ; i < 4 ; ++ i) {
            const vector_t & storage = n[3 + i].as<vector_t>();

            for (int idx = 0 ; idx < static_cast<int>(rhs->getBundleSize()[0]) ; ++ idx) {
                for (int idy = 0 ; idy < static_cast<int>(rhs->getBundleSize()[1]) ; ++ idy) {
                    const index_t bi({idx, idy});

                    if (const typename cslibs_ndt_2d::static_maps::Gridmap::distribution_bundle_t* b =
                            rhs->getDistributionBundle(bi))
                        b->at(i)->data() = storage[get_index(bi, i)];
                }
            }
        }

        return true;
    }
};
}

#endif // CSLIBS_NDT_2D_SERIALIZATION_STATIC_MAPS_GRIDMAP_HPP
