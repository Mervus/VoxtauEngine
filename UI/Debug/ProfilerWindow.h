//
// Created by Claude on 03/02/2026.
//

#ifndef VOXTAU_PROFILERWINDOW_H
#define VOXTAU_PROFILERWINDOW_H

#include <vector>
#include <string>

class ProfilerWindow {
public:
    ProfilerWindow();

    void Draw();

    void SetOpen(bool open) { _isOpen = open; }
    bool IsOpen() const { return _isOpen; }

private:
    bool _isOpen = true;
    bool _sortByTime = true;      // Sort by time (true) or alphabetically (false)
    bool _showGraph = true;       // Show time history graph
    float _graphHeight = 60.0f;   // Height of the graph
};

#endif //VOXTAU_PROFILERWINDOW_H
