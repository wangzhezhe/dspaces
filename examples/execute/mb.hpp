#ifndef __MB_HEADER
#define __MB_HEADER

#include <cmath>
#include <fstream>
#include <iostream>
#include <mpi.h>
#include <vector>

// the camara view need to be reset to get better picture of the rendered results
static unsigned Globalpid = 0;

#define MANDELBULB_ALLGATHER_TAG 2001
#define MANDELBULB_BARRIER_TAG 2002
#define MANDELBULB_BCAST_TAG 2003
#define MONA_UNDEFINED -1

class Mandelbulb
{

  struct Vector
  {
    double x;
    double y;
    double z;
  };

  inline int& value_at(unsigned x, unsigned y, unsigned z)
  {
    return m_data[z + m_depth * (y + m_height * x)];
  }

  static inline double do_r(const Vector& v)
  {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  }

  static inline double do_theta(double n, const Vector& v) { return n * std::atan2(v.y, v.x); }

  static inline double do_phi(double n, const Vector& v) { return n * std::asin(v.z / do_r(v)); }

  static int iterate(const Vector& v0, double order, unsigned max_cycles)
  {
    Vector v = v0;
    double n = order;
    int i;
    for (i = 0; i < max_cycles && (v.x * v.x + v.y * v.y + v.z * v.z < 2.0); i++)
    {
      double r = do_r(v);
      double theta = do_theta(n, v);
      double phi = do_phi(n, v);
      double rn = std::pow(r, n);
      double cos_theta = std::cos(theta);
      double sin_theta = std::sin(theta);
      double cos_phi = std::cos(phi);
      double sin_phi = std::sin(phi);
      Vector vn = { rn * cos_theta * cos_phi, rn * sin_theta * cos_phi, -rn * sin_phi };
      v.x = vn.x + v0.x;
      v.y = vn.y + v0.y;
      v.z = vn.z + v0.z;
    }
    return i;
  }

public:
  Mandelbulb(){};
  Mandelbulb(unsigned width, unsigned height, unsigned depth, double z_offset, float range,
    int blockid, unsigned nblocks = 1)
    : m_width(width)
    , m_height(height)
    , m_depth(depth + 1)
    , m_extents(6)
    , m_origin(3)
    , m_data(width * height * (depth + 1))
    , m_z_offset(z_offset)
    , m_range(range)
    , m_nblocks(nblocks)
    , m_blockid(blockid)
  {
    m_extents[4] = 0;
    m_extents[5] = m_width - 1;
    m_extents[2] = 0;
    m_extents[3] = m_height - 1;
    m_extents[0] = 0;
    m_extents[1] = m_depth - 1;
    m_origin[2] = 0;
    m_origin[1] = 0;
    m_origin[0] = z_offset*1.0 / (nblocks*1.0);
  }

  Mandelbulb(const Mandelbulb&) = default;
  Mandelbulb(Mandelbulb&&) = default;
  Mandelbulb& operator=(const Mandelbulb&) = default;
  Mandelbulb& operator=(Mandelbulb&&) = default;
  ~Mandelbulb() = default;

  void compute(double order, unsigned max_cycles = 100)
  {
    for (unsigned z = 0; z < m_depth; z++)
      for (unsigned y = 0; y < m_height; y++)
        for (unsigned x = 0; x < m_width; x++)
        {
          Vector v = { 2.0 * m_range * x / m_width - m_range,
            2.0 * m_range * y / m_height - m_range,
            2.0 * m_range * (m_z_offset + z) / ((m_depth - 1) * m_nblocks) - m_range };
          value_at(x, y, z) = iterate(v, order, max_cycles);
        }
  }

  void writeBIN(const std::string& filename)
  {
    std::ofstream ofile(filename.c_str(), std::ios::binary);
    ofile.write((char*)m_data.data(), sizeof(int) * m_data.size());
  }

  int* GetExtents() const { return const_cast<int*>(m_extents.data()); }

  double* GetOrigin() const { return const_cast<double*>(m_origin.data()); }

  int DataSize() const { return m_data.size(); }

  int* GetData() const { return const_cast<int*>(m_data.data()); }

  int GetBlockID() const { return m_blockid; }

  // copy it to data by memory operation
  void SetData(std::vector<char>& stageData)
  {
    size_t byteSize = stageData.size();
    if (byteSize != this->m_data.size() * sizeof(int))
    {
      throw std::runtime_error("wrong data length, bytesize " + std::to_string(byteSize) +
        " cell size " + std::to_string(this->m_data.size()));
    }
    int* array = static_cast<int*>((void*)stageData.data());
    // replace current m_data
    memcpy(&(this->m_data[0]), array, byteSize);
  }

  int GetNumberOfLocalCells() const { return m_data.size(); }

  unsigned GetZoffset() const { return m_z_offset; }

  bool compare(Mandelbulb& other)
  {
    if (other.m_width != m_width || other.m_height != m_height || other.m_depth != m_depth)
    {
      std::cout << "unequal part1" << std::endl;
      return false;
    }
    if (other.m_z_offset != m_z_offset || other.m_range != m_range || other.m_nblocks != m_nblocks)
    {
      std::cout << "unequal part2" << std::endl;
      return false;
    }
    if (other.m_extents.size() != m_extents.size() || other.m_origin.size() != m_origin.size() ||
      other.m_data.size() != m_data.size())
    {
      std::cout << "unequal part3" << std::endl;
      return false;
    }
    for (int i = 0; i < m_extents.size(); i++)
    {
      if (other.m_extents[i] != m_extents[i])
      {
        std::cout << "unequal m_extents" << std::endl;
        return false;
      }
    }
    for (int i = 0; i < m_origin.size(); i++)
    {
      if (other.m_origin[i] != m_origin[i])
      {
        std::cout << "unequal m_origin" << std::endl;
        return false;
      }
    }
    for (int i = 0; i < m_data.size(); i++)
    {
      if (other.m_data[i] != m_data[i])
      {
        std::cout << "unequal m_data" << std::endl;
        return false;
      }
    }
    return true;
  }

  /*
   template <typename A>
   void serialize(A& ar)
   {
     ar(m_width, m_height, m_depth, CEREAL_NVP(m_extents), CEREAL_NVP(m_origin), CEREAL_NVP(m_data),
       m_z_offset, m_range, m_nblocks);
   }
   */
private:
  size_t m_width;
  size_t m_height;
  size_t m_depth;
  std::vector<int> m_extents;
  std::vector<double> m_origin;
  std::vector<int> m_data;
  unsigned m_z_offset;
  float m_range;
  unsigned m_nblocks;
  int m_blockid;
};

#endif
