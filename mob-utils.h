/**
* @file mob-utils.h
* @brief mob utils
* @version 0.1
* @date 2022-04-22
* 
* @author: Sérgio Vieira - sergio.vieira@ifce.edu.br
**/

#ifndef MOB_UTILS
#define MOB_UTILS

#include <fstream>
#include <iostream>
#include <sstream> 
#include <unordered_set>
#include <unordered_map>

/**
 * @brief node
 * 
 * @param id 
 * @param x 
 * @param y 
 * @param z 
 */
struct node {
    uint32_t id = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/**
 * @brief mob node
 * 
 * @param node_id 
 * @param time 
 * @param dst_x 
 * @param dst_y 
 * @param dst_z 
 */
struct mob {
    uint32_t node_id = 0;
    double time = 0.0;
    double dst_x = 0.0;
    double dst_y = 0.0;
    double dst_z = 0.0;
};

/**
 * @brief mob info
 * 
 * @param start_time 
 * @param end_time 
 * @param nodes 
 */
struct mob_info {
    double start_time = 0.0;
    double end_time = 0.0;
    uint32_t nodes = 0;
};

using NodeMap = std::unordered_map<uint32_t, node>;

std::ifstream open_file(std::string_view file) {
    std::ifstream inputFile(file.data()); // Open the file in text mode
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << file << std::endl;
        exit(EXIT_FAILURE);
    }
    return inputFile;
}

mob_info get_mob_info(std::string_view file) {
    mob_info info;
    std::ifstream inputFile = open_file(file);
    std::string line;
    double currentTime = 0.0;
    std::unordered_set<uint32_t> uniqueNodeIds;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        size_t pos = command.find("$node_(");
        if (pos != std::string::npos) {
            size_t epos = command.find(")", pos);
            uint32_t nodeId; 
            if (epos != std::string::npos) {
                nodeId = std::atoi(command.substr(pos + 7, (epos - pos) - 1).c_str());
            } else {
                std::cerr << "Error parsing node ID" << std::endl;
                continue; // Skip
            }
            uniqueNodeIds.insert(nodeId);
        }
        
        pos = line.find("$ns_ at ");
        if (pos != std::string::npos) {
            command = line.substr(pos + 8);
            size_t epos = command.find(" ");
            double time = 0.0;
            if (epos != std::string::npos) { 
                time = std::atof(command.substr(0, epos).c_str());
            } else {
                std::cerr << "Error parsing time" << std::endl;
                continue; // Skip
            }
            if (info.start_time == 0.0 || time < info.start_time) {
                info.start_time = time;
            }

            if (time > info.end_time) {
                info.end_time = time;
            }
        }
    }
    info.nodes = uniqueNodeIds.size();
    inputFile.close();
    return info;
}

std::string get_mob_info_str(const mob_info& info) {
    std::stringstream ss;
    ss << "Nodes: " << info.nodes << "\n";
    ss << "Start time: " << info.start_time << "\n";
    ss << "End time: " << info.end_time << "\n";
    return ss.str();
}

NodeMap make_nodes_from_file(std::string_view file) {
    std::ifstream input = open_file(file);
    std::unordered_map<uint32_t, node> nodes_map;
    std::string line;
    while (std::getline(input, line)) {
        if (line.find("setdest") != std::string::npos) continue;
        size_t pos = line.find("$node_(");
        if (pos != std::string::npos) {\
            // read node id
            size_t epos = line.find(")", pos);
            uint32_t nodeId; 
            if (epos != std::string::npos) {
                nodeId = std::atoi(line.substr(pos + 7, (epos - pos) - 1).c_str());
            } else {
                std::cerr << "Error parsing node ID" << std::endl;
                continue; // Skip
            }
            if (nodes_map.find(nodeId) == nodes_map.end()) {
                nodes_map[nodeId] = node{id:nodeId};
            }
            // read node's positions
            size_t pos_ = line.find("_", epos);
            epos = line.size();
            if (pos_ != std::string::npos) {
                double value = std::stod(line.substr(pos_ + 1, (epos - pos_) - 1).c_str());
                if (line[pos_ - 1] == 'X') {
                    nodes_map[nodeId].x = value;
                } else if (line[pos_ - 1] == 'Y') {
                    nodes_map[nodeId].y = value;
                } else if (line[pos_ - 1] == 'Z') {
                    nodes_map[nodeId].z = value;
                }
            } else {
                std::cerr << "Error parsing node position" << std::endl;
                continue; // Skip
            }
        }
    }
    input.close();
    return nodes_map;
}

std::ostream& operator<<(std::ostream& out, const node& node) {
    out << "node{id:"
        << node.id << ", x:"
        << node.x << ", y:" 
        << node.y << ", z:"
        << node.z << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const NodeMap& map) {
    out << "[\n";
    size_t i = 0;
    for (auto [key, value]: map) {
        out << "  " << value;
        if (i < map.size() - 1) out << ",";
        out << '\n';
    }
    out << "]";
    return out;
}

#endif