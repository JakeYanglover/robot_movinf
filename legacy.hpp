

size_t findNearestFinishedStation(int64_t itemFilterBit, double posX, double posY, bool buff, bool debuff)
{
    size_t selectedStationIndex = SIZE_MAX;
    double selectedDis = 0.0;
    for (size_t si = 0; si < stationCount; si++) {
        if (((1 << stations[si].type) & itemFilterBit) == 0)
        {
            continue;
        }
        if (stations[si].reachingFinished || stations[si].remaining == -1)
        {
            continue;
        }
        double dis = getDistance(posX - stations[si].posX, posY - stations[si].posY);
        if (buff)
        {
            dis -= getStationDisBonus(si);
        }
        if (debuff)
        {
            dis += getRouteRobotDisBonus(posX, posY, si);
        }
        if (selectedStationIndex == SIZE_MAX || dis < selectedDis)
        {
            selectedStationIndex = si;
            selectedDis = dis;
        }
    }
    return selectedStationIndex;
}
