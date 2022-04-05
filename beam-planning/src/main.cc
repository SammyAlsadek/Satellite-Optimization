#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <future>
#include <mutex>

#include "../header/classes.h"

// globals
const uint8_t MAX_BEAMS = 32;
const uint8_t SAME_COLOR_LIMIT = 10; // >= 10
const uint8_t INTERF_SAT_LIMIT = 20; // >= 20
const uint8_t USER_SAT_LIMIT = 45;   // =< 45

std::mutex mtx;

// function prototypes
void process_file(std::string &file_name, std::vector<User> &user_vector, std::vector<Starlink_Sat> &starlink_vector, std::vector<Interferer_Sat> &interferer_vector);
bool valid_connection_vertical(User &user, Starlink_Sat &starlink);
bool valid_connection_interferer(User &user, Starlink_Sat &starlink, std::vector<Interferer_Sat> &interferer_vector);
bool valid_connection_interferer_helper(User &user, Starlink_Sat &starlink, Interferer_Sat &interferer);
bool valid_connection_color(char &color, User &user1, Starlink_Sat &starlink);
bool valid_connection_color_helper(User &user1, User &user2, Starlink_Sat &starlink);
double get_degrees(std::vector<double> a, std::vector<double> b, std::vector<double> c);
void print_connections(std::vector<Starlink_Sat> &starlink_vector);
void make_connections(char &color, std::vector<User> &user_vector, std::vector<Starlink_Sat> &starlink_vector, std::vector<Interferer_Sat> &interferer_vector);

int main(int argc, char **args)
{
    std::string file_name = args[1];
    std::vector<User> user_vector;
    std::vector<Starlink_Sat> starlink_vector;
    std::vector<Interferer_Sat> interferer_vector;
    char colors[] = {'A', 'B', 'C', 'D'};

    process_file(file_name, user_vector, starlink_vector, interferer_vector);

    // main logic of the program
    for (auto &color : colors)
    {
        std::future fut = std::async(std::launch::async, make_connections, std::ref(color), std::ref(user_vector), std::ref(starlink_vector), std::ref(interferer_vector));
    }

    print_connections(starlink_vector);

    return 0;
}

void make_connections(char &color, std::vector<User> &user_vector, std::vector<Starlink_Sat> &starlink_vector, std::vector<Interferer_Sat> &interferer_vector)
{
        for (auto &sat : starlink_vector)
        {
            for (auto &user : user_vector)
            {
                std::lock_guard<std::mutex> lck (mtx);

                if (sat.get_beams().size() >= MAX_BEAMS)
                    break;

                int id = user.get_id();

                if (sat.get_beams().find(id) != sat.get_beams().end() || !valid_connection_vertical(user, sat) || !valid_connection_color(color, user, sat) || user.get_connected() || !valid_connection_interferer(user, sat, interferer_vector))
                    continue;

                // make connection
                sat.get_beams().insert(std::make_pair(id, std::make_pair(user, color)));
                user.set_connected(true);
            }
        }
}

void print_connections(std::vector<Starlink_Sat> &starlink_vector)
{
    for (auto &sat : starlink_vector)
    {
        int i = 0;
        for (auto &beam : sat.get_beams())
        {
            std::cout << "sat " << sat.get_id() << " beam " << ++i << " user " << beam.first << " color " << beam.second.second << std::endl;
        }
    }
}

bool valid_connection_color(char &color, User &new_user, Starlink_Sat &starlink)
{
    for (auto &beam : starlink.get_beams())
    {
        if (color != beam.second.second)
            continue;

        if (!valid_connection_color_helper(new_user, beam.second.first, starlink))
            return false;
    }

    return true;
}

bool valid_connection_color_helper(User &user1, User &user2, Starlink_Sat &starlink)
{
    // Here we will be calculating the to see if two beams of the same color are at least 10 degrees apart
    return (180 - get_degrees(user1.get_position(), starlink.get_position(), user2.get_position())) >= SAME_COLOR_LIMIT;
}

bool valid_connection_interferer(User &user, Starlink_Sat &sat, std::vector<Interferer_Sat> &interferer_vector)
{
    for (auto &interferer : interferer_vector)
        if (!valid_connection_interferer_helper(user, sat, interferer))
            return false;

    return true;
}

bool valid_connection_interferer_helper(User &user, Starlink_Sat &starlink, Interferer_Sat &interferer)
{
    // Here we will be calculating the to see if non-starlink beams are further than 20 degrees of the startlink beams
    return (180 - get_degrees(starlink.get_position(), user.get_position(), interferer.get_position())) >= INTERF_SAT_LIMIT;
}

bool valid_connection_vertical(User &user, Starlink_Sat &starlink)
{
    // Here we will be calculating the to see if the beam is of 45 degrees of the vertical.
    std::vector<double> center_of_earth;
    center_of_earth.push_back(0);
    center_of_earth.push_back(0);
    center_of_earth.push_back(0);
    return (get_degrees(starlink.get_position(), user.get_position(), center_of_earth)) <= USER_SAT_LIMIT;
}

double get_degrees(std::vector<double> a, std::vector<double> b, std::vector<double> c)
{
    double ab[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    double bc[3] = {c[0] - b[0], c[1] - b[1], c[2] - b[2]};

    double abVec = std::sqrt(ab[0] * ab[0] + ab[1] * ab[1] + ab[2] * ab[2]);
    double bcVec = std::sqrt(bc[0] * bc[0] + bc[1] * bc[1] + bc[2] * bc[2]);

    double abNorm[3] = {ab[0] / abVec, ab[1] / abVec, ab[2] / abVec};
    double bcNorm[3] = {bc[0] / bcVec, bc[1] / bcVec, bc[2] / bcVec};

    double res = abNorm[0] * bcNorm[0] + abNorm[1] * bcNorm[1] + abNorm[2] * bcNorm[2];

    return (std::acos(res) * 180.0 / 3.141592653589793);
}

void process_file(std::string &file_name, std::vector<User> &user_vector, std::vector<Starlink_Sat> &starlink_vector, std::vector<Interferer_Sat> &interferer_vector)
{
    std::ifstream input_file(file_name);

    if (input_file.is_open())
    {
        std::string buffer;

        while (std::getline(input_file, buffer))
        {
            // skip comments and empty lines
            if (buffer[0] == '#' || buffer.length() == 0)
                continue;

            std::string type;
            int id;
            double x, y, z;
            std::vector<double> position;

            std::stringstream line(buffer);
            line >> type >> id >> x >> y >> z;
            position.push_back(x);
            position.push_back(y);
            position.push_back(z);

            if (type == "user")
            {
                User entity(id, position);
                user_vector.push_back(entity);
            }
            else if (type == "sat")
            {
                Starlink_Sat entity(id, position);
                starlink_vector.push_back(entity);
            }
            else if (type == "interferer")
            {
                Interferer_Sat entity(id, position);
                interferer_vector.push_back(entity);
            }
        }

        input_file.close();
    }
}