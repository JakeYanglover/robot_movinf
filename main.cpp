#define _CRT_SECURE_NO_WARNINGS
#define PI (3.14159265358979323846)

#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <cstdio>

/// 1.确定每个机器人的目标（优先级越高就越先卖，把需求按照优先级排起来，再安排机器人）
/// 2.移动，碰撞规避

enum GlobalMode {
    Normal,
    Clean,
};

struct Station
{
    /// [1, 9]
    int64_t type;
    /// Binary
    /// 0b10: has 1
    /// 0b110: has 1 and 2
    int64_t materialRequirementBit;
    double posX, posY;
    /// -1: Empty
    /// 0: Done
    /// >=0: Has remaining time(frames)
    int64_t remaining;
    /// Binary
    /// 0b10: has 1
    /// 0b110: has 1 and 2
    int64_t materialStatusBit;
    int64_t finished;

    /// Online value
    /// Please update it when refresh.
    bool reachingFinished;
    bool reachingMaking;
    int64_t reachingMaterialBit;
    void refresh()
    {
        this->reachingMaking = false;
        this->reachingFinished = false;
        this->reachingMaterialBit = 0;
    }
};

struct Robot
{
    /// The station this robot is near of. -1 is None.
    int64_t stationNearbyIndex;
    double timeRatio, bumpRatio;
    double posX, posY;
    /// [-PI, PI)
    ///  (0 is right, -PI/2 is up, PI/2 is down)
    double angle;
    double velX, velY;
    /// [-PI, PI)
    ///  (0 is right, -PI/2 is up, PI/2 is down)
    double velAngle;

    /// 0 - nothing
    /// 1-7 has something
    int64_t takingType;

    /// Flag value
    int64_t flagEndFrame = -1;
    double flagRotate = -1;
    double flagForward = -1;

    /// Online value
    /// Please update it when refresh.
    size_t targetStationIndex;
    bool ctrlBuy;
    bool ctrlSell;
    bool ctrlDestroy;
    double ctrlForward;
    double ctrlRotate;
    void refresh()
    {
        targetStationIndex = SIZE_MAX;
        this->ctrlBuy = false;
        this->ctrlSell = false;
        this->ctrlDestroy = false;
        this->ctrlForward = 0.0;
        this->ctrlRotate = 0.0;
    }
};

const uint64_t TYPE_REQUIREMENTS[] = {
    0b00000000, 
    0b00000000, 0b00000000, 0b00000000,
    0b00000110, 0b00001010, 0b00001100,
    0b01110000, 0b10000000, 0b11110000,
};
const size_t ROBOT_COUNT = 4, STATION_MAX_COUNT = 500, REQUIREMENT_MAX_COUNT = 10000;
const double MAX_SPEED = 6.0;


GlobalMode globalMode = Normal;
int64_t frameId = 0, coinNow = 0;
int64_t stationCount = 0;
Robot robots[ROBOT_COUNT];
int64_t robotCount = 4;
Station stations[STATION_MAX_COUNT];

bool loadMap();
bool readInputScanf();
bool setRobotTarget();
bool setRobotMovements();

char cache[10000];
signed main()
{
    loadMap();
    // Take control
    while (true) {
        bool inputResult = readInputScanf();
        if (!inputResult)
        {
            break;
        }
        setRobotTarget();
        setRobotMovements();
    }
    return EXIT_SUCCESS;
}

bool loadMap()
{
    for (size_t i = 0; i < 101; i++) {
        fscanf(stdin, "%s", cache);
    }
    // Output OK
    fprintf(stdout, "OK\n");
    fflush(stdout);
    return true;
}

/*
 * Frame Part 1: Input.
 */

bool readInputScanf()
{
    int64_t i1, i2, i3, i4;
    double d1, d2, d3, d4, d5, d6, d7, d8;
    // int64_t frameId;
    // int64_t stationCount;
    int i = fscanf(stdin, "%lld %lld", &frameId, &i1);
    if (i == EOF) {
        return false;
    }
    fscanf(stdin, "%lld", &stationCount);
    for (size_t ri = 0; ri < stationCount; ri++) {
        fscanf(stdin,
            "%lld %lf %lf %lld %lld %lld",
            &i1, &d1, &d2, &i2, &i3, &i4);
        stations[ri].type = i1;
        stations[ri].posX = d1;
        stations[ri].posY = d2;
        stations[ri].remaining = i2;
        stations[ri].materialStatusBit = i3;
        stations[ri].finished = i4;
        stations[ri].materialRequirementBit = TYPE_REQUIREMENTS[stations[ri].type];
        stations[ri].refresh();
    }
    robotCount = ROBOT_COUNT;
    for (size_t ri = 0; ri < robotCount; ri++) {
        fscanf(stdin,
            "%lld %lld %lf %lf %lf %lf %lf %lf %lf %lf",
            &i1, &i2, &d1, &d2, &d3, &d4, &d5, &d6, &d7, &d8);
        robots[ri].stationNearbyIndex = i1;
        // robots[i].takingStatusBit = (i2 > 0)? (1 << i2): 0;
        robots[ri].takingType = i2;
        robots[ri].timeRatio = d1;
        robots[ri].bumpRatio = d2;
        robots[ri].velAngle = d3;
        robots[ri].velX = d4;
        robots[ri].velY = d5;
        robots[ri].angle = d6;
        robots[ri].posX = d7;
        robots[ri].posY = d8;
        robots[ri].refresh();
    }
    fscanf(stdin, "%s", cache);
    return true;
}

/*
 * Frame Part 2: Set robot target.
 */
const size_t ITEM_TYPE_COUNT = 10;
int64_t itemCounts[ITEM_TYPE_COUNT];
int64_t itemOnStationCounts[ITEM_TYPE_COUNT];
size_t itemOnStations[ITEM_TYPE_COUNT][STATION_MAX_COUNT];
int64_t itemOnRobotCounts[ITEM_TYPE_COUNT];
size_t itemOnRobots[ITEM_TYPE_COUNT][ROBOT_COUNT];

int64_t itemOnStationsF1Count = 0;
size_t itemOnStationsF1[STATION_MAX_COUNT];
int64_t itemOnStationsF2Count = 0;
size_t itemOnStationsF2[STATION_MAX_COUNT];
int64_t itemOnStationsF3Count = 0;
size_t itemOnStationsF3[STATION_MAX_COUNT];

size_t itemOnStationTotal[STATION_MAX_COUNT];
size_t itemOnStationTotalCount = 0;

void refreshItemCounts()
{
    // Clear
    for (int64_t item = 1; item <= 7; ++item)
    {
        itemCounts[item] = 0;
        itemOnRobotCounts[item] = 0;
        itemOnStationCounts[item] = 0;
    }
    itemOnStationsF1Count = 0;
    itemOnStationsF2Count = 0;
    itemOnStationsF3Count = 0;
    itemOnStationTotalCount = 0;
    // Robots
    for (size_t ri = 0; ri < robotCount; ri++) {
        if (robots[ri].takingType > 0)
        {
            itemCounts[robots[ri].takingType] += 1;
            itemOnRobots[robots[ri].takingType][itemOnRobotCounts[robots[ri].takingType]] = ri;
            itemOnRobotCounts[robots[ri].takingType] += 1;
        }
    }
    // Stations
    for (size_t si = 0; si < stationCount; si++) {
        int64_t stype = stations[si].type;
        if (stations[si].finished || stations[si].remaining != -1)
        {
            itemCounts[stype] += 1;
            itemOnStations[stype][itemOnStationCounts[stype]] = si;
            itemOnStationCounts[stype] += 1;
            itemOnStationTotal[itemOnStationTotalCount] = si;
            itemOnStationTotalCount += 1;
        }
        for (int64_t item = 1; item <= 7; item += 1)
        {
            if ((stations[si].materialStatusBit & (1 << item)) != 0)
            {
                itemCounts[item] += 1;
            }
        }
    }
    // Stations Floor
    for (int64_t item = 1; item <= 3; ++item)
    {
        for (size_t i = 0; i < itemOnStationCounts[item]; ++i)
        {
            itemOnStationsF1[itemOnStationsF1Count] = itemOnStations[item][i];
            itemOnStationsF1Count += 1;
        }
    }
    for (int64_t item = 4; item <= 6; ++item)
    {
        for (size_t i = 0; i < itemOnStationCounts[item]; ++i)
        {
            itemOnStationsF2[itemOnStationsF2Count] = itemOnStations[item][i];
            itemOnStationsF2Count += 1;
        }
    }
    for (int64_t item = 7; item <= 7; ++item)
    {
        for (size_t i = 0; i < itemOnStationCounts[item]; ++i)
        {
            itemOnStationsF3[itemOnStationsF3Count] = itemOnStations[item][i];
            itemOnStationsF3Count += 1;
        }
    }
    // if (frameId % 100 == 0)
    // {
    //     fprintf(stderr, "ic4-6: %lld, %lld, %lld\n",
    //             itemCounts[4], itemCounts[5], itemCounts[6]);
    // }
}

double min(double a, double b) { return (a < b)? a: b; }

/**
 * [-PI, PI)
 */
double getAngle(double deltaX, double deltaY)
{
    if (deltaX == 0.0)
    {
        return (deltaY > 0.0)? PI/2: -PI/2;
    }
    if (deltaY == 0.0)
    {
        return (deltaX > 0.0)? 0: -PI;
    }
    double angleTan = deltaY / deltaX;
    double angleUp = std::atan(angleTan);
    if (deltaX < 0.0)
    {
        if (angleUp > 0.0)
        {
            angleUp -= PI;
        }
        else {
            angleUp += PI;
        }
    }
    return angleUp;
}

/**
 * [-PI, PI)
 */
double getDeltaAngle(double fromAngle, double toAngle)
{
    double deltaAngle = toAngle - fromAngle;
    while (deltaAngle >= PI)
    {
        deltaAngle -= 2 * PI;
    }
    while (deltaAngle < -PI)
    {
        deltaAngle += 2 * PI;
    }
    return deltaAngle;
}

double getDistance(double deltaX, double deltaY)
{
    // use std::sqrt;
    return sqrt(deltaX*deltaX + deltaY*deltaY);
}

size_t getRouteRobotCount(double posX, double posY, size_t tsi)
{
    size_t count = 0;
    double angle2t = getAngle(stations[tsi].posX - posX, 
            stations[tsi].posY - posY);
    for (size_t ri = 0; ri < robotCount; ri += 1)
    {
        double angle2r = getAngle(robots[ri].posX - posX,
                robots[ri].posY - posY);
        double deltaAngle = angle2r - angle2t;
        while (deltaAngle > PI)
        {
            deltaAngle -= 2 * PI;
        }
        while (deltaAngle < -PI)
        {
            deltaAngle += 2 * PI;
        }
        if (abs(deltaAngle) < PI/4)
        {
            count += 1;
        }
    }
    return count;
}

double getTimeExpected(double posX, double posY, double angle, double velAngle, size_t nsi)
{
    double deltaX = stations[nsi].posX - posX;
    double deltaY = stations[nsi].posY - posY;
    double dis = getDistance(deltaX, deltaY);
    double deltaAngle = getDeltaAngle(angle, getAngle(deltaX, deltaY));
    // double deltaAngle = getDeltaAngle(angle + velAngle/10.0, getAngle(deltaX, deltaY));
    // return dis / MAX_SPEED + (deltaAngle / PI * 3.0);
    return dis / MAX_SPEED;
}

double getScore(double posX, double posY, double angle, double velAngle, size_t nsi)
{
    double score = 0.0;
    // Time comsuming
    double timeExpected = getTimeExpected(posX, posY, angle, velAngle, nsi);
    score -= 10.0 * timeExpected;
    // Empty
    if (stations[nsi].remaining == -1)
    {
        score += 50.0;
    }
    if (timeExpected > 5.0)
    {
        return score;
    }
    // Has material
    for (int64_t status = (stations[nsi].materialStatusBit | stations[nsi].reachingMaterialBit);
            status > 0;
            status >>= 1)
    {
        if ((status & 1) != 0)
        {
            score += 30.0;
        }
    }
    // Needed
    if (4 <= stations[nsi].type && stations[nsi].type <= 6)
    {
        int64_t typeCount = itemCounts[4] + itemCounts[5] + itemCounts[6];
        int64_t remainingCount = typeCount - itemCounts[stations[nsi].type];
        score += 10.0 * remainingCount;
    }
    if (stations[nsi].type == 7)
    {
        int64_t typeCount = itemCounts[4] + itemCounts[5] + itemCounts[6];
        score += min(100.0, 15.0 * typeCount);
    }
    if (stations[nsi].type == 8)
    {
        int64_t typeCount = itemCounts[4] + itemCounts[5] + itemCounts[6];
        score += min(100.0, 20.0 * typeCount);
    }
    if (stations[nsi].type == 9)
    {
        score -= 50.0;
        int64_t typeCount = itemCounts[4] + itemCounts[5] + itemCounts[6];
        score += min(100.0, 15.0 * typeCount);
    }
    return score;
}

size_t findBestNeededStation(int64_t itemType, double posX, double posY, double angle, double velAngle) {
    size_t selectedStationIndex = SIZE_MAX;
    double nowscore = 0.0;
    for (size_t si = 0; si < stationCount; si++) {
        int64_t stationNowRequirementBit = 
            ((stations[si].materialRequirementBit ^ stations[si].materialStatusBit)
                ^ stations[si].reachingMaterialBit);
        if (((1 << itemType) & stationNowRequirementBit) == 0)
        {
            continue;
        }
        double dis = getDistance(posX - stations[si].posX, posY - stations[si].posY);
        double score = getScore(posX, posY, angle, velAngle, si);
        if (selectedStationIndex == SIZE_MAX
                || score > nowscore)
        {
            selectedStationIndex = si;
            nowscore = score;
        }
    }
    return selectedStationIndex;
}

void doFloor(int64_t scount, size_t *ps)
{
    for (size_t cnt = 0; cnt < scount*2; cnt += 1)
    {
        size_t nowsi = SIZE_MAX, nownsi = SIZE_MAX, nowri = SIZE_MAX;
        double nowscore = 0.0;
        for (size_t iosi = 0; iosi < scount; iosi += 1)
        {
            size_t tsi = ps[iosi];
            bool finishedRemain = (stations[tsi].finished && (!stations[tsi].reachingFinished));
            bool makingRemain = (stations[tsi].remaining != -1 && (!stations[tsi].reachingMaking));
            if ((!finishedRemain) && (!makingRemain))
            {
                continue;
            }
            for (size_t nsi = 0; nsi < stationCount; nsi++) {
                int64_t stationNowRequirementBit = 
                    ((stations[nsi].materialRequirementBit ^ stations[nsi].materialStatusBit)
                        ^ stations[nsi].reachingMaterialBit);
                if (((1 << stations[tsi].type) & stationNowRequirementBit) == 0)
                {
                    continue;
                }
                for (size_t ri = 0; ri < robotCount; ri += 1)
                {
                    if (robots[ri].takingType != 0 || robots[ri].targetStationIndex != SIZE_MAX)
                    {
                        continue;
                    }
                    double dis = getDistance(
                            stations[tsi].posX - robots[ri].posX,
                            stations[tsi].posY - robots[ri].posY
                            );
                    double ndis = getDistance(
                            stations[nsi].posX - stations[tsi].posX,
                            stations[nsi].posY - stations[tsi].posY
                            );
                    if (!stations[tsi].finished)
                    {
                        if (MAX_SPEED * (stations[tsi].remaining / 50.0) > dis)
                        {
                            continue;
                        }
                    }
                    if ((9000 - frameId) / 50.0 * MAX_SPEED < dis + ndis + 3.0)
                    {
                        continue;
                    }
                    double score =  - dis + getScore(
                            stations[tsi].posX, stations[tsi].posY, robots[ri].angle, robots[ri].velAngle,
                            nsi);
                    if (nowsi == SIZE_MAX || score > nowscore)
                    {
                        nowsi = tsi;
                        nownsi = nsi;
                        nowri = ri;
                        nowscore = score;
                    }
                }
            }
        }
        if (nowsi != SIZE_MAX)
        {
            bool finishedRemain = (stations[nowsi].finished && (!stations[nowsi].reachingFinished));
            bool makingRemain = (stations[nowsi].remaining != -1 && (!stations[nowsi].reachingMaking));
            if (finishedRemain)
            {
                robots[nowri].targetStationIndex = nowsi;
                stations[nowsi].reachingFinished = true;
                stations[nownsi].reachingMaterialBit |= (1 << stations[nowsi].type);
            }
            else if (makingRemain)
            {
                robots[nowri].targetStationIndex = nowsi;
                stations[nowsi].reachingMaking = true;
                stations[nownsi].reachingMaterialBit |= (1 << stations[nowsi].type);
            }
        }
    }
}

bool setRobotTarget()
{
    // refresh
    refreshItemCounts();
    // if has item, find target.
    for (size_t ri = 0; ri < robotCount; ++ri)
    {
        if (robots[ri].takingType == 0)
        {
            continue;
        }
        size_t tsi = findBestNeededStation(
                // robots[ri].takingType, robots[ri].posX, robots[ri].posY, true
                robots[ri].takingType, robots[ri].posX, robots[ri].posY, robots[ri].angle, robots[ri].velAngle
                );
        if (tsi != SIZE_MAX)
        {
            robots[ri].targetStationIndex = tsi;
            stations[tsi].reachingMaterialBit |= (1 << robots[ri].takingType);
        }
    }
    // if has no item, filter and get nearest.
    // doFloor(itemOnStationsF3Count, itemOnStationsF3);
    // doFloor(itemOnStationsF2Count, itemOnStationsF2);
    // doFloor(itemOnStationsF1Count, itemOnStationsF1);
    doFloor(itemOnStationTotalCount, itemOnStationTotal);
    return true;
}

/*
 * Frame Part 3: (While Output) Set robot movement.
 */

void setRobotOnlineValues()
{
    for (size_t ri = 0; ri < robotCount; ++ri)
    {
        size_t tsi = robots[ri].targetStationIndex;
        if (tsi == SIZE_MAX)
        {
            robots[ri].ctrlForward = 6.0;
            robots[ri].ctrlRotate = PI;
            // Destroy
            if (robots[ri].takingType != 0)
            {
                robots[ri].ctrlDestroy = true;
            }
            continue;
        }
        // Angle
        double deltaX = stations[tsi].posX - robots[ri].posX;
        double deltaY = stations[tsi].posY - robots[ri].posY;
        double deltaAngle = getDeltaAngle(robots[ri].angle, getAngle(deltaX, deltaY));
        double speedAngle = 0.0;
        if (deltaAngle > 0.1)
        {
            speedAngle = PI;
        }
        if (deltaAngle < -0.1)
        {
            speedAngle = -PI;
        }
        robots[ri].ctrlRotate = speedAngle;
        // Speed
        double disFromTarget = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        double speed = 0.0;
        if (disFromTarget >= 3.0 && abs(deltaAngle) < (PI/2)*(0.8))
        {
            speed = MAX_SPEED;
        }
        else if (disFromTarget >= 1.0 && disFromTarget < 3.0 && abs(deltaAngle) < (PI/2)*(0.4))
        {
            speed = MAX_SPEED * ((PI/2 - abs(deltaAngle))/(PI/2));
        }
        else if (disFromTarget < 1.0)
        {
            speed = (MAX_SPEED * (disFromTarget) / 1.0) * 0.6;
        }
        // Speed: Anti wall
        if (robots[ri].takingType != 0 && speed == MAX_SPEED)
        {
            double aX = cos(robots[ri].angle);
            double aY = sin(robots[ri].angle);
            double posX = robots[ri].posX;
            double posY = robots[ri].posY;
            if (posX < 5.0 && aX < 0.0)
            {
                speed *= (abs(posX)-0.0)/(5.0) * (1.0-abs(aX));
            }
            if (posX > 45.0 && aX > 0.0)
            {
                speed *= (50.0-abs(posX))/(5.0) * (1.0-abs(aX));
            }
            if (posY < 5.0 && aY < 0.0)
            {
                speed *= (abs(posY)-0.0)/(5.0) * (1.0-abs(aY));
            }
            if (posY > 45.0 && aY > 0.0)
            {
                speed *= (50.0-abs(posY))/(5.0) * (1.0-abs(aY));
            }
        }
        robots[ri].ctrlForward = speed;
        // Sell
        bool sell = false;
        if ((robots[ri].stationNearbyIndex != -1)
                && (robots[ri].stationNearbyIndex == robots[ri].targetStationIndex))
        {
            sell = (robots[ri].takingType != 0);
        }
        robots[ri].ctrlSell = sell;
        // Purchase
        bool purchase = false;
        if ((robots[ri].stationNearbyIndex != -1)
                && (robots[ri].stationNearbyIndex == robots[ri].targetStationIndex))
        {
            purchase = (robots[ri].takingType == 0);
        }
        robots[ri].ctrlBuy = purchase;
    }
}

void antiBump()
{
    for (size_t ri = 0; ri < robotCount; ++ri)
    {
        for (size_t pri = 0; pri < ri; pri += 1)
        {
            // antiBump Check: Direction
            double deltaDirectionAngle = getDeltaAngle(robots[pri].angle + PI, robots[ri].angle);
            if (robots[ri].takingType == 0 && robots[pri].takingType == 0)
            {
                if (abs(deltaDirectionAngle) > (PI)*(0.1))
                {
                    continue;
                }
            }
            else
            {
                if (abs(deltaDirectionAngle) > (PI)*(0.8))
                {
                    continue;
                }
            }
            // antiBump Check: Distance
            double deltaX = robots[pri].posX - robots[ri].posX;
            double deltaY = robots[pri].posY - robots[ri].posY;
            double rvelX = robots[ri].velX;
            double rvelY = robots[ri].velY;
            rvelX = cos(robots[ri].angle);
            rvelY = sin(robots[ri].angle);
            double ty = (deltaX*rvelX + deltaY*rvelY) / std::sqrt(rvelX*rvelX + rvelY*rvelY);
            double dis = std::sqrt(deltaX*deltaX + deltaY*deltaY);
            double ldis = std::sqrt(dis*dis - ty*ty);
            if (std::isnan(ty) || ty < 0.0 || ty > 5.0 || ldis > 1.0)
            {
                continue;
            }
            if (ldis/ty > 0.5)
            {
                continue;
            }
            // antiBump Add
            size_t protectri = SIZE_MAX, avoidri = SIZE_MAX;
            if (robots[ri].takingType != 0 && robots[pri].takingType == 0)
            {
                protectri = ri;
                avoidri = pri;
            }
            else
            {
                protectri = pri;
                avoidri = ri;
            }
            // antiBump
            if (frameId < robots[avoidri].flagEndFrame)
            {
                continue;
            }
            double protectAngle = getAngle(
                    robots[protectri].posX - robots[avoidri].posX,
                    robots[protectri].posY - robots[avoidri].posY);
            double deltaProtectAngle = protectAngle - robots[ri].angle;
            robots[avoidri].flagEndFrame = frameId + 15;
            robots[avoidri].flagForward = MAX_SPEED;
            if (deltaProtectAngle > 0.0)
            {
                robots[avoidri].flagRotate = -PI;
            }
            else {
                robots[avoidri].flagRotate = PI;
            }
            // fprintf(stderr, "antiBump[%llu]:%llu dis:%.2lf ty:%.2lf ldis:%.2lf taking:%lld ratio:%.2lf %.2lf\n",
            //         frameId, avoidri, dis, ty, ldis, robots[avoidri].takingType,
            //         robots[avoidri].timeRatio, robots[avoidri].bumpRatio);
        }
    }
}

void useFlag()
{
    for (size_t ri = 0; ri < robotCount; ++ri)
    {
        if (frameId < robots[ri].flagEndFrame)
        {
            robots[ri].ctrlForward = robots[ri].flagForward;
            robots[ri].ctrlRotate = robots[ri].flagRotate;
        }
    }
}

void printCtrl()
{
    for (size_t ri = 0; ri < robotCount; ++ri)
    {
        fprintf(stdout, "forward %llu %lf\n", ri, robots[ri].ctrlForward);
        fprintf(stdout, "rotate %llu %lf\n", ri, robots[ri].ctrlRotate);
        if (robots[ri].ctrlSell)
        {
            fprintf(stdout, "sell %llu\n", ri);
        }
        if (robots[ri].ctrlBuy)
        {
            fprintf(stdout, "buy %llu\n", ri);
        }
        if (robots[ri].ctrlDestroy)
        {
            fprintf(stdout, "destroy %llu\n", ri);
        }
    }
}

bool setRobotMovements()
{
    // Cmp
    setRobotOnlineValues();
    antiBump();
    useFlag();
    // Print Part 1
    fprintf(stdout, "%llu\n", frameId);
    // Print Part 2
    printCtrl();
    // Print Final
    fprintf(stdout, "OK\n");
    fflush(stdout);
    return true;
}
