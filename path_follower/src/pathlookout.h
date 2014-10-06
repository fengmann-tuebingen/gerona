#ifndef PATHLOOKOUT_H
#define PATHLOOKOUT_H

/// STL
#include <string>

/// THIRD PARTY
#include <opencv2/imgproc/imgproc.hpp>

/// ROS
#include <sensor_msgs/LaserScan.h>
#include <nav_msgs/OccupancyGrid.h>
#include <tf/transform_listener.h>

/// PROJECT
#include "path.h"
#include "maptransformer.h"
#include "visualizer.h"
#include "obstacletracker.h"
#include <path_msgs/FollowPathFeedback.h>

class PathFollower;


/**
 * @brief Looks out for obstacles on the path.
 *
 * Looks for obstacles farther ahead, that are blocking the path. Keeps track of such obstacles and weights them,
 * depending on how long the obstacle is visible and how far away it is.
 * If the weight of any obstacle exceeds a defined limit, an alarm is risen so the robot can early react e.g. by
 * replaning.
 *
 * The weight is calculated in weightObstacle(). It increases quadratically with increasing lifetime and decreasing
 * distance.
 * There are two parameters to adjust the weight:
 *    ~obstacle_scale_distance: Distance at which the robot stops, independed of the duration-weight.
 *    ~obstacle_scale_lifetime: Duration after which the robot stops, independend of the distance-weight.
 */
class PathLookout
{
public:
    PathLookout(PathFollower *node);

    void setFrontScan(const sensor_msgs::LaserScanConstPtr &msg);
    void setBackScan(const sensor_msgs::LaserScanConstPtr &msg);

    void setMap(const nav_msgs::OccupancyGridConstPtr &msg);

    //! Set the path, which is to be checked for obstacles.
    void setPath(const PathWithPosition &path);

    //! Check if there is an obstacle on the path ahead of the robot, that gives a reason to cancel the current path.
    /**
     * @return True, if there is an obstacle that justifies the abortion of the path.
     */
    bool lookForObstacles(path_msgs::FollowPathFeedback *feedback);

    //! Reset the obstacle tracker (should be called before starting a new path).
    void reset();

private:
    struct Options
    {
        float scale_obstacle_distance_;
        float scale_obstacle_lifetime_;
        //! Stop robot, if the weight of an obstacle becomes higher than this value.
        float obstacle_weight_limit_;

        //! Width of the path in meters.
        float path_width_;
    } opt_;

    //! TF-Frame in which the obstacles are tracked (should be independent of the robots movement, thus /map is a good choise).
    std::string obstacle_frame_;

    PathFollower *node_;
    MapTransformer map_trans_;
    ObstacleTracker tracker_;
    Visualizer *visualizer_;
    tf::TransformListener tf_listener_;

    sensor_msgs::LaserScanConstPtr front_scan_;
    sensor_msgs::LaserScanConstPtr back_scan_;

    //! The current obstacle map.
    nav_msgs::OccupancyGridConstPtr map_;

    //! Current map converted to cv::Mat.
    cv::Mat map_image_;

    Path path_;

    //! Mask image of the path
    cv::Mat path_image_;

    void configure();

    std::vector<Obstacle> lookForObstaclesInMap();

    std::vector<Obstacle> lookForObstaclesInScans();

    std::vector<std::vector<cv::Point2f> > clusterPoints(const std::vector<cv::Point2f> &points);

    //! draw the path to the path image (assumes, that map is already set!)
    /** @see path_image_ */
    void drawPathToImage(const Path &path);

    //! Compute weight for the given obstacle, depending on its distance to the robot and its lifetime.
    float weightObstacle(cv::Point2f robot_pos, ObstacleTracker::TrackedObstacle o) const;

    std::vector<cv::Point2f> findObstacleInScan(const sensor_msgs::LaserScanConstPtr &scan);
};

#endif // PATHLOOKOUT_H
