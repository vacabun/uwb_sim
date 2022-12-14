#include "uwb_locate.hpp"

UWBLocate::UWBLocate() : Node("uwb_locate")
{
    this->load_anchors_pos();

    subscription_ = this->create_subscription<uwb_interfaces::msg::UWBData>(
        "/uwbData", 10, std::bind(&UWBLocate::topic_callback, this, std::placeholders::_1));

    msgPublisher_ = this->create_publisher<uwb_interfaces::msg::UWBLocationData>("uwbLocationRes", 10);
}

void UWBLocate::topic_callback(const uwb_interfaces::msg::UWBData::SharedPtr msg)
{

    int64_t robotId = msg->robot_id;

    std::unordered_map<int, double> uwbDistance;
    for (long unsigned int i = 0; i < msg->distances.size(); i++)
    {
        uwbDistance[msg->distances[i].anthor_id] = msg->distances[i].distance;
    }

    Position3D estimatedRes;
    LOCATOR_RETURN calState;
    calState = calculate_pos_robust_ransac(anthorPoseMap, uwbDistance, estimatedRes);
    if (calState == CALCULATE_SUCCESS)
    {
        RCLCPP_INFO(this->get_logger(), "uwb location success. res: x: %f, y: %f", calState, estimatedRes.x, estimatedRes.y);
    }
    else
    {
        RCLCPP_INFO(this->get_logger(), "uwb location failed (type: %d). res: x: %f, y: %f", calState, estimatedRes.x, estimatedRes.y);
    }

    uwb_interfaces::msg::UWBLocationData data;

    data.set__robot_id(robotId);

    data.set__x(estimatedRes.x);
    data.set__y(estimatedRes.y);

    msgPublisher_->publish(data);
}

void UWBLocate::load_anchors_pos()
{
    std::string packageShareDirectory = ament_index_cpp::get_package_share_directory("uwb_location");

    std::string anthorConfigFilePath = packageShareDirectory + "/config/anthor.xml";

    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(anthorConfigFilePath.c_str()) != 0)
    {
        RCLCPP_INFO(this->get_logger(), "load anthor config file failed.");
    }
    else
    {
        RCLCPP_INFO(this->get_logger(), "load anthor config file successed.");
    }

    tinyxml2::XMLElement *anthor = doc.RootElement()->FirstChildElement("anthor");

    while (anthor)
    {
        int id = atoi(anthor->FirstAttribute()->Value());

        tinyxml2::XMLElement *attr = anthor->FirstChildElement();

        // geometry_msgs::msg::Point p;
        // p.set__x(std::stod(attr->GetText()));
        // attr = attr->NextSiblingElement();
        // p.set__y(std::stod(attr->GetText()));
        // attr = attr->NextSiblingElement();
        // p.set__z(std::stod(attr->GetText()));

        Position3D p;
        p.x = std::stod(attr->GetText());
        attr = attr->NextSiblingElement();
        p.y = std::stod(attr->GetText());
        attr = attr->NextSiblingElement();
        p.z = std::stod(attr->GetText());

        anthorPoseMap[id] = p;

        RCLCPP_INFO(this->get_logger(), "load anthor id: %d position:%f %f %f", id, anthorPoseMap[id].x, anthorPoseMap[id].y, anthorPoseMap[id].z);

        anthor = anthor->NextSiblingElement();
    }
}

