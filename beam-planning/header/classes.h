#pragma once

class Entity
{
private:
    int id;
    std::vector<double> position;

public:
    Entity(const int id, const std::vector<double> position) : id(id), position(position)
    {
    }

    int get_id() const
    {
        return this->id;
    }
    std::vector<double> get_position() const
    {
        return this->position;
    }

    void set_id(const int id)
    {
        this->id = id;
    }
    void set_position(const std::vector<double> position)
    {
        this->position = position;
    }
};

class User : public Entity
{
private:
    bool connected;

public:
    User(const int id, const std::vector<double> position) : connected(false), Entity(id, position)
    {
    }

    bool get_connected() const
    {
        return this->connected;
    }

    void set_connected(const bool state)
    {
        this->connected = state;
    }
};

class Starlink_Sat : public Entity
{
private:
    std::map<int, std::pair<User, char> > beams;

public:
    Starlink_Sat(const int id, const std::vector<double> position) : Entity(id, position)
    {
    }

    std::map<int, std::pair<User, char> > &get_beams()
    {
        return this->beams;
    }
};

class Interferer_Sat : public Entity
{
private:
public:
    Interferer_Sat(const int id, const std::vector<double> position) : Entity(id, position)
    {
    }
};