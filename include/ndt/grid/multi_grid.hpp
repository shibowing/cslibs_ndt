#ifndef MULTI_GRID_HPP
#define MULTI_GRID_HPP

#include <ndt/grid/mask.hpp>
#include <ndt/grid/grid.hpp>
#include <ndt/data/pointcloud.hpp>

namespace ndt {
namespace grid {
template<std::size_t Dim>
class MultiGrid {
public:
    typedef std::shared_ptr<MultiGrid<Dim>>  Ptr;
    typedef typename Grid<Dim>::Size         Size;
    typedef typename Grid<Dim>::Index        Index;
    typedef typename Grid<Dim>::Resolution   Resolution;
    typedef typename Grid<Dim>::Point        Point;
    typedef typename Grid<Dim>::Distribution Distribution;

    typedef typename Grid<Dim>::Matrix       Matrix;
    typedef std::vector<Distribution*>          Distributions;

    MultiGrid() :
        data_size(0),
        data(nullptr)
    {
    }
    /// NOTICE :
    /// Displacement of 0.25 * resolution results in 0.5 displacement from grids to each other
    /// Origin of Multigrid is in the center of the combined grid area.

    /// maybe point cloud constructor

    MultiGrid(const Size       &_size,
                 const Resolution &_resolution,
                 const Point      &_origin = Point::Zero()) :
        size(_size),
        resolution(_resolution),
        origin(_origin),
        data_size(mask.rows),
        normalizer(1.0 / data_size),
        data(new Grid<Dim>[data_size])
    {
        Resolution offsets;
        for(std::size_t i = 0 ; i < Dim; ++i) {
            offsets[i] = +_resolution[i] * 0.25;
        }

        for(std::size_t i = 0 ; i < data_size ; ++i) {
            Point o = origin;
            for(std::size_t j = 0 ; j < Dim ; ++j) {
                o(j) += mask[i * mask.cols + j] * offsets[j];
            }
            data[i] = Grid<Dim>(_size, _resolution, o);
        }

        steps[0] = 1;
        if(Dim > 1) {
            std::size_t max_idx = Dim - 1;
            for(std::size_t i = 1 ; i <= max_idx ; ++i) {
                steps[i] = steps[i-1] * 2;
            }
        }
    }

    MultiGrid(const MultiGrid &other) :
        size(other.size),
        resolution(other.resolution),
        origin(other.origin),
        data_size(other.data_size),
        data(new Grid<Dim>[data_size])
    {
        std::memcpy(data, other.data, sizeof(Grid<Dim>) * data_size);
    }

    MultiGrid & operator = (const MultiGrid &other)
    {
        if(this != &other) {
            std::size_t former_size = size;
            size = other.size;
            resolution = other.resolution;
            origin = other.origin;
            data_size = other.data_size;
            if(size != former_size) {
                delete [] data;
                data = new Grid<Dim>[data_size];
            }
            std::memcpy(data, other.data, sizeof(Grid<Dim>) * data_size);
        }
        return *this;
    }

    virtual ~MultiGrid()
    {
        delete[] data;
        data = nullptr;
    }

    /// ---------------- META INFORMATION ---------------- ///
    inline Size getSize() const
    {
        return size;
    }

    inline void getSize(Size &_size) const
    {
        _size = size;
    }

    inline Resolution getResolution() const
    {
        return resolution;
    }

    inline void getResolution(Resolution &_resolution) const
    {
        _resolution = resolution;
    }

    inline bool checkIndex(const Index &_index)
    {
        std::size_t p = pos(_index);
        return p < data_size;
    }

    inline Point getOrigin() const
    {
        return origin;
    }

    /// ---------------- INSERTION ---------------------------- ///
    inline bool add(const Point &_p)
    {
        bool result = false;
        for(std::size_t i = 0 ; i < data_size; ++i) {
            result |= data[i].add(_p);
        }
        return result;
    }

    inline bool add(const data::Pointcloud<Dim> &_p)
    {
        bool result = false;
        for(std::size_t i = 0 ; i < _p.size ; ++i) {
            if(_p.mask[i] == data::Pointcloud<Dim>::VALID) {
                for(std::size_t j = 0 ; j < data_size; ++j)
                    result |= data[j].add(_p.points[i]);
            }
        }
        return result;
    }

    /// ---------------- SAMPLING ----------------------------- ///
    inline double sample(const Point &_p)
    {
        double result = 0.0;
        for(std::size_t i = 0 ; i < data_size ; ++i) {
            Distribution *distr = data[i].get(_p);
            if(distr != nullptr)
                result += distr->evaluate(_p);
        }
        return result;
    }

    inline double sampleNonNormalized(const Point &_p)
    {
        double result = 0.0;
        for(std::size_t i = 0 ; i < data_size ; ++i) {
            Distribution *distr = data[i].get(_p);
            if(distr != nullptr)
                result += distr->evaluateNonNoramlized(_p);
        }
        return result;
    }


    inline void get(const Point         &_p,
                    Distributions       &_distributions)
    {
        _distributions.clear();
        for(std::size_t i = 0 ; i < data_size ; ++i) {
            Distribution *distr = data[i].get(_p);
            if(distr != nullptr)
                _distributions.push_back(distr);
        }
    }

    inline Grid<Dim> const & at(const Index &_index) const
    {
        std::size_t p = pos(_index);
        if(p >= data_size)
            throw std::runtime_error("Out of bounds!");

        return data[p];
    }

    inline Grid<Dim> & at(const Index &_index)
    {
        std::size_t p = pos(_index);
        if(p >= data_size)
            throw std::runtime_error("Out of bounds!");

        return data[p];
    }

private:
    Size          size;
    Size          steps;
    Resolution    resolution;
    Point         origin;
    std::size_t   data_size;
    double        normalizer;
    Grid<Dim> *data;

    Mask<Dim>     mask;

    inline std::size_t pos(const Index &_index) {
        std::size_t p = 0;
        for(std::size_t i = 0 ; i < Dim ; ++i) {
            p += steps[i] * _index[i];
        }
        return p;
    }

};
}
}
#endif // MULTI_GRID_HPP