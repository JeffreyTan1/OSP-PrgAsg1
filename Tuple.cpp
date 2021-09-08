#include <tuple>
#include <queue>
#include <iostream>

typedef std::tuple<int, int, int> barberTuple;

std::priority_queue<barberTuple, std::vector<barberTuple>, std::greater<barberTuple>> barberPQueue;

int main(void)
{
    barberTuple bt = std::make_tuple(10, 10, 10);
    barberPQueue.push(bt);
    std::cout << std::get<0>(bt);
}