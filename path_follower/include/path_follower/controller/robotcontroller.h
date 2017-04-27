#ifndef ROBOTCONTROLLER_H
#define ROBOTCONTROLLER_H

// THIRD PARTY
#include <ros/node_handle.h>
#include <geometry_msgs/Twist.h>
#include <Eigen/Core>
#include <visualization_msgs/Marker.h>
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Float64MultiArray.h"
#include <nav_msgs/Path.h>
#include <cmath>

// PROJECT
#include <path_follower/utils/path.h>
#include <path_follower/utils/movecommand.h>
#include <path_follower/utils/path_interpolated.h>
#include <path_follower/utils/parameters.h>


class PoseTracker;
class Visualizer;
class CoursePredictor;
class ObstacleAvoider;

class PathFollowerParameters;

class RobotController
{
    /* DATA */
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    enum class ControlStatus
    {
        OKAY,          //!< Everything is okay, the robot is still driving.
        OBSTACLE,      //!< The obstacle avoider is active and modified the move command.
        REACHED_GOAL,  //!< Goal is reached. Path execution is finished.
        ERROR          //!< Some error occured. Path execution is aborted.
    };

    /* ABSTRACT METHODS */
public:
    //! Immediatley stop any motion.
    virtual void stopMotion() = 0;

    //! Return true, if the robot is capable of omnidirectional movement.
    virtual bool isOmnidirectional() const;

    //! Is called when the execution of a new path is started. Can be used for initialization stuff.
    virtual void start() {}

    //! Is called at several points, if new path is set or movement is aborted.
    virtual void reset();

    //! Set the robot pose
    virtual void setCurrentPose(const Eigen::Vector3d&) {}

    virtual void precomputeSteerCommand(Waypoint& wp_now,  Waypoint& wp_next ) {}

    virtual void initialize();

    bool reachedGoal(const Eigen::Vector3d& pose) const;

protected:
    //! This is a subset of ControlStatus. computeMoveCommand is not allowed to report obstacles
    enum class MoveCommandStatus
    {
        OKAY, REACHED_GOAL, ERROR
    };

    /**
     * @brief Computes the next move command.
     * @param cmd Output. The move command for this iteration.
     * @return Status that can be used to indicate errors. `OKAY` if everything is ok.
     */
    virtual MoveCommandStatus computeMoveCommand(MoveCommand* cmd) = 0;

    /**
     * @brief Converts the move command to ros message and publishs it.
     *
     * The implementation has to convert the move command to the appropriate ROS message and
     * publish it using `this->cmd_pub_`.
     * Use initPublisher() to initialize `this->cmd_pub_`.
     *
     * @see cmd_pub_
     * @see initPublisher
     *
     * @param cmd The move command.
     */
    virtual void publishMoveCommand(const MoveCommand &cmd) const = 0;


    /**
     * @brief Initialize the command publisher
     *
     * Takes a pointer to a publisher instance and initializes it. The default behaviour is to
     * set up an publisher that sends `geometry_msgs::Twist` messages to "/cmd_vel".
     * Overwrite this method if you need an other message type and/or topic.
     *
     * @param pub Output parameter
     */
    virtual void initPublisher(ros::Publisher* pub) const;


    ////interpolation///

    struct InterpolationParameters : public Parameters {
        P<double> goal_tolerance;

        InterpolationParameters() :
            goal_tolerance(this, "~goal_tolerance", 0.3, "minimum distance at which the robot stops")
        {}
    };

    virtual const InterpolationParameters& getParameters() const = 0;

    ////-------------////

    virtual double findOrthogonalProjection();
    virtual bool isGoalReached(MoveCommand *cmd);


    /* REGULAR METHODS */
public:
    RobotController();

    virtual void init(PoseTracker* pose_tracker, ObstacleAvoider* obstacle_avoider, const PathFollowerParameters* options);

    virtual ~RobotController() {}

    //! Execute one iteration of path following. This method should not be overwritten by subclasses!
    ControlStatus execute();

    //! Set the global path (map frame)
    virtual void setPath(Path::Ptr path);

    //! Set the local path (odometry frame)
    virtual void setGlobalPath(Path::Ptr path);

    virtual void setVelocity(float v)
    {
        ROS_WARN_STREAM("setting velocity to " << v);
        velocity_ = v;
    }

    //! Set +1 for forward movement and -1 if the robot is driving backward.
    virtual void setDirSign(float s)
    {
        dir_sign_ = s;
    }

    virtual float getDirSign() const
    {
        return dir_sign_;
    }

    std::string getFixedFrame() const;

    void computeMovingDirection();

private:
    //Vizualize the path driven by the robot
    void publishPathMarker();

protected:
    ros::NodeHandle nh_;
    ros::NodeHandle pnh_;

    ros::Publisher cmd_pub_;
    ros::Publisher points_pub_;

    PoseTracker* pose_tracker_;
    ObstacleAvoider* obstacle_avoider_;

    const PathFollowerParameters* global_opt_;

    Visualizer *visualizer_;

    //! Desired velocity (defined by the action goal).
    float velocity_;

    //! Indicates the direction of movement (>0 -> forward, <0 -> backward)
    float dir_sign_;

    //! Current path.
    Path::Ptr path_;
    PathInterpolated global_path_;
    //! The next waypoint in the robot frame (set by setPath).
    Eigen:: Vector3d next_wp_local_;


    //! Calculate the angle between the orientations of the waypoint and the robot.
    virtual double calculateAngleError();

    //! Convert a MoveCommandStatus to its corresponding ControlStatus
    static ControlStatus MCS2CS(MoveCommandStatus s);

    //path driven by the robot
    visualization_msgs::Marker robot_path_marker_;

    ///interpolation////

    ros::Publisher interp_path_pub_;


    PathInterpolated path_interpol;

    // is there an interpolated path?
    bool interpolated_;

    //interpolated path
    nav_msgs::Path interp_path_;

    void publishInterpolatedPath();
    ////-------------////

    //index of the orthogonal projection to the path
    uint proj_ind_;
};

#endif // ROBOTCONTROLLER_H
