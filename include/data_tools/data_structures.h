#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <Eigen/Dense>
#include <eigen_cereal/eigen_cereal.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/string.hpp>
#include <boost/filesystem.hpp>

#include <sparse_gp/sparse_gp.h>
#include <sparse_gp/rbf_kernel.h>
#include <sparse_gp/gaussian_noise.h>

struct mbes_ping
{
    // data structure used to store a collection of mbes_pings
    using PingsT = std::vector<mbes_ping, Eigen::aligned_allocator<mbes_ping> >;

    unsigned int id_; // unique ID of swath
    std::string time_string_; // readable time stamp string
    long long time_stamp_; // posix time stamp
    double heading_; // heading of vehicle
    double heave_; // heave of vehicle
    double pitch_; // pitch of vehicle
    double roll_; // roll of vehicle
    bool first_in_file_; // is first entry in file?
    Eigen::Vector3d pos_; // NOTE: this comes from associating ping with nav data

    std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d> > beams; // all 3d points in swath

	template <class Archive>
    void serialize( Archive & ar )
    {
        ar(CEREAL_NVP(id_), CEREAL_NVP(time_string_), CEREAL_NVP(time_stamp_), CEREAL_NVP(heading_),
		   CEREAL_NVP(heave_), CEREAL_NVP(pitch_), CEREAL_NVP(roll_), CEREAL_NVP(first_in_file_),
           CEREAL_NVP(pos_), CEREAL_NVP(beams));
    }

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct nav_entry
{
    // data structure used to store a collection of nav_entries
    using EntriesT = std::vector<nav_entry, Eigen::aligned_allocator<nav_entry> >;

    std::string time_string_; // readable time stamp string
    long long time_stamp_; // posix time stamp
    bool first_in_file_; // is first entry in a file?
    Eigen::Vector3d pos_; // 3d position of vehicle
	
    template <class Archive>
    void serialize( Archive & ar )
    {
        ar(CEREAL_NVP(time_string_), CEREAL_NVP(time_stamp_), CEREAL_NVP(first_in_file_), CEREAL_NVP(pos_));
    }

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

struct pt_submaps
{
    using PointsT = std::vector<Eigen::MatrixXd, Eigen::aligned_allocator<Eigen::MatrixXd> >;
    using TransT = std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d> >;
    using RotsT = std::vector<Eigen::Matrix3d, Eigen::aligned_allocator<Eigen::Matrix3d> >;
    using AngsT = std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d> >;
    using MatchesT = std::vector<std::pair<int, int> >; // tells us which maps overlap
    using BoundsT = std::vector<Eigen::Matrix2d, Eigen::aligned_allocator<Eigen::Matrix2d> >;

    PointsT points; // Nx3 matrices with all points in the submaps
    TransT trans; // translation of submaps
    RotsT rots; // rotation matrices of submaps, same as angles
    AngsT angles; // euler angles of submaps, same as rots
    MatchesT matches; // overlapping submap matches, containing vector indices of matches
    MatchesT binary_constraints; // consecutive submaps, containing vector indices of matches
    BoundsT bounds; // bounds of the submap, bb(0, 0) - min x, bb(0, 1) - min y, bb(1, 0) - max x, bb(1, 1) - max y
    
    template <class Archive>
    void serialize( Archive & ar )
    {
        ar(CEREAL_NVP(points), CEREAL_NVP(trans), CEREAL_NVP(rots),
           CEREAL_NVP(angles), CEREAL_NVP(matches), CEREAL_NVP(bounds));
    }
};

struct gp_submaps : public pt_submaps
{
    using ProcessT = sparse_gp<rbf_kernel, gaussian_noise>;
    using SubmapsGPT = std::vector<ProcessT>; // Process does not require aligned allocation as all matrices are dynamic

    SubmapsGPT gps;
    
    template <class Archive>
    void serialize( Archive & ar )
    {
        ar(cereal::base_class<pt_submaps>(this), CEREAL_NVP(gps));
    }
};

template <typename T>
T read_data(const boost::filesystem::path& path)
{
    if (!boost::filesystem::exists(path)) {
        std::cout << "File " << path << " does not exist..." << std::endl;
        exit(0);
    }

    T rtn;
    std::ifstream is(path.string(), std::ifstream::binary);
    {
        cereal::BinaryInputArchive archive(is);
        archive(rtn);
    }
    is.close();

    return rtn;
}

template <typename T>
void write_data(T& data, const boost::filesystem::path& path)
{
    std::ofstream os(path.string(), std::ofstream::binary);
	{
		cereal::BinaryOutputArchive archive(os);
        archive(data);
	}
    os.close();
}

#endif // DATA_STRUCTURES_H
