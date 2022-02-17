#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui-SFML.h"
#include <SFML/Graphics.hpp>
#include "Floormap.hpp"
#include "Controller.hpp"
#include "Algorithms.hpp"
#include "Constants.hpp"
#include "Units.hpp"
#include <iostream>
#include <algorithm>
#include <string>
#include <math.h>
#include <iomanip>

// 30 feet = 9.144 meters
// 1 box = 3.415 pixels/box
// 1 box = 0.5544342178 meters/box
// 9.144 meters in pixels = 56.32184847 pixels

// States of the create/modify point modal
enum modifyPointState { CREATE, MODIFY, NONE};

// Current Map Pointer Object
std::shared_ptr<fsim::Floormap> map;

// Floor levels
// enum FloorLabel { GROUND = 0, SECOND = 1, THIRD = 2, FOURTH = 3 };

// Current Floor Enum Level
FloorLabel currentEnumFloor = FloorLabel::GROUND;

// Floor labels
const std::vector<std::pair<uint32_t, std::string>> FloorLevels =
    { std::make_pair(1, "(Ground)"), std::make_pair(2, "(2nd)"), std::make_pair(3, "(3rd)"), std::make_pair(4, "(4th)") };

// Map texture paths
const std::vector<std::string> mapTexturePaths =
    { "resource/Ground-2160.png", "resource/2nd-2160.png", "resource/3rd-2160.png", "resource/4th-2160.png" };

// Stores the current modal point state
static modifyPointState modalModify = modifyPointState::NONE;

// current floor label
std::pair<uint32_t, std::string> currentLevel = FloorLevels[currentEnumFloor];

// Holds all starting points that are dynamically created
// static std::vector<fsim::StartingPoints*> startingPoints;

// Calculated paths of all the exits in a given node
static std::vector<std::pair<fsim::Node*, uint32_t>> exitsStored;

// Temporary starting point pointer
fsim::StartingPoints* startingPointTemp = nullptr;

// Current map texture
sf::Texture* currentMapTexture = nullptr;

// Distance of one column with relative to the map
static const float pixelDistance = 0.4878571428f;

// States to avoid creating c strings every loop to display current loop
static bool startingPointsChanged = true;
static char startingPointsCString[19];
static bool floorChanged = true;
static char floorCString[24];

// General states
static bool enableWASD = false;
static bool startPointMoving = false;
static bool mouseOnImGui = false;
static bool mouseDown = false;
static int  screenClickHandle = 0;
static float heatFluxValue = 200;

// Modal states 
static bool alpha_preview = true;
static bool alpha_half_preview = false;
static bool drag_and_drop = true;
static bool options_menu = true;
static bool hdr = false;

// Marker call back helper functions
typedef void (*ImGuiDemoMarkerCallback)(const char* file, int line, const char* section, void* user_data);
extern ImGuiDemoMarkerCallback  GImGuiDemoMarkerCallback;
extern void*                    GImGuiDemoMarkerCallbackUserData;
ImGuiDemoMarkerCallback         GImGuiDemoMarkerCallback = NULL;
void*                           GImGuiDemoMarkerCallbackUserData = NULL;
#define IMGUI_DEMO_MARKER(section)  do { if (GImGuiDemoMarkerCallback != NULL) GImGuiDemoMarkerCallback(__FILE__, __LINE__, section, GImGuiDemoMarkerCallbackUserData); } while (0)


// Help marker
static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
// Flags for modify/create point modal
ImGuiColorEditFlags misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (drag_and_drop ? 0 : ImGuiColorEditFlags_NoDragDrop) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);

// Displays create/modify point modal
static void displayModifyModal(const modifyPointState& state)
{
    std::string option;
    if (state == modifyPointState::CREATE)
    {
        option = "Add Starting Point";
    }
    else if (state == modifyPointState::MODIFY)
    {
        option = "Modify Point";
    }
    const char* option_cstr = option.c_str();
    ImGui::OpenPopup(option_cstr);
    if (ImGui::BeginPopupModal(option_cstr, NULL, ImGuiWindowFlags_Modal))
    {
        if (ImGui::Button("Close"))
        {
            if (modalModify == modifyPointState::CREATE)
            {
                delete startingPointTemp;
                startingPointTemp = nullptr;
                std::cout << "Cancelled Starting Node" << std::endl;
            }
            else if (modalModify == modifyPointState::MODIFY)
            {
                std::cout << "Cancelled Modify Node" << std::endl;
                startingPointTemp = nullptr;

            }
            modalModify = modifyPointState::NONE;
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }
        if (modalModify == modifyPointState::CREATE)
        {
            if (startingPointTemp == nullptr)
            {
                startingPointTemp = new fsim::StartingPoints();
                std::cout << "Created" << std::endl;
            }
        }


        ImGui::InputText("Input Label", startingPointTemp->buffer, 7);
        ImGui::Separator();
        static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 255.0f / 255.0f);

        IMGUI_DEMO_MARKER("Widgets/Color/ColorPicker");
        static bool alpha = true;
        static bool alpha_bar = true;
        static bool side_preview = true;
        static bool ref_color = false;
        static ImVec4 ref_color_v(1.0f, 0.0f, 1.0f, 0.5f);
        static int display_mode = 0;
        static int picker_mode = 0;
        ImGui::Checkbox("With Alpha", &alpha);
        ImGui::Checkbox("With Alpha Bar", &alpha_bar);
        ImGui::Checkbox("With Side Preview", &side_preview);
        if (side_preview)
        {
            ImGui::SameLine();
            ImGui::Checkbox("With Ref Color", &ref_color);
            if (ref_color)
            {
                ImGui::SameLine();
                ImGui::ColorEdit4("##RefColor", &ref_color_v.x, ImGuiColorEditFlags_NoInputs | misc_flags);
            }
        }
        ImGui::Combo("Display Mode", &display_mode, "Auto/Current\0None\0RGB Only\0HSV Only\0Hex Only\0");
        ImGui::SameLine(); HelpMarker(
            "ColorEdit defaults to displaying RGB inputs if you don't specify a display mode, "
            "but the user can change it with a right-click.\n\nColorPicker defaults to displaying RGB+HSV+Hex "
            "if you don't specify a display mode.\n\nYou can change the defaults using SetColorEditOptions().");
        ImGui::Combo("Picker Mode", &picker_mode, "Auto/Current\0Hue bar + SV rect\0Hue wheel + SV triangle\0");
        ImGui::SameLine(); HelpMarker("User can right-click the picker to change mode.");
        ImGuiColorEditFlags flags = misc_flags;
        if (!alpha)            flags |= ImGuiColorEditFlags_NoAlpha;        // This is by default if you call ColorPicker3() instead of ColorPicker4()
        if (alpha_bar)         flags |= ImGuiColorEditFlags_AlphaBar;
        if (!side_preview)     flags |= ImGuiColorEditFlags_NoSidePreview;
        if (picker_mode == 1)  flags |= ImGuiColorEditFlags_PickerHueBar;
        if (picker_mode == 2)  flags |= ImGuiColorEditFlags_PickerHueWheel;
        if (display_mode == 1) flags |= ImGuiColorEditFlags_NoInputs;       // Disable all RGB/HSV/Hex displays
        if (display_mode == 2) flags |= ImGuiColorEditFlags_DisplayRGB;     // Override display mode
        if (display_mode == 3) flags |= ImGuiColorEditFlags_DisplayHSV;
        if (display_mode == 4) flags |= ImGuiColorEditFlags_DisplayHex;

        ImGui::ColorPicker4("Color##4", (float*)&color, flags, ref_color ? &ref_color_v.x : NULL);
        
        if (ImGui::Button("Save"))
        {
            if (modalModify == modifyPointState::CREATE)
                map->startingPoints.push_back(startingPointTemp);

            modalModify = modifyPointState::NONE;
            std::cout << "Saved" << std::endl;
            startingPointTemp->point_rgba = color;

            startingPointTemp = nullptr;
            startingPointsChanged = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

static void loadMapTexture(fsim::Floormap& map, const FloorLabel& floor)
{
    if (currentMapTexture != nullptr)
        delete currentMapTexture;
    
    currentMapTexture = new sf::Texture();
    currentMapTexture->loadFromFile(mapTexturePaths[floor]);
    currentMapTexture->setSmooth(true);
    map.setMapTexture(currentMapTexture);
}

static void modifyNodes(fsim::Floormap& map, std::function<void(fsim::Node*)> operations)
{
    for (size_t i = 0; i < map.getTotalRows(); ++i)
    {
        for (size_t k = map.minCols; k < map.maxCols; ++k)
        {
            operations(map.nodes[i][k]);
        }
    }
}

int main()
{
    auto videoMode = sf::VideoMode::getDesktopMode();
    videoMode.height += 1;
    // sf::ContextSettings settings;
    // settings.antialiasingLevel = 4;
    sf::RenderWindow window(videoMode, "Window", sf::Style::None);
    bool imGuiInit = ImGui::SFML::Init(window);

    if (imGuiInit)
        std::cout << "ImGui Success!" << std::endl;

    ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    sf::Vector2f ImGuiWindowSize((float)((float)300.0f/768.0f) * (videoMode.height), videoMode.height);

    const float boardWidth = 1366.0f;
    const uint32_t totalCols = 400;
    const float tileSize = boardWidth / (float)totalCols;
    uint32_t totalRows = std::ceil((float)768.0f / tileSize);
    std::vector<fsim::Node*>* nodes = new std::vector<fsim::Node*>[totalRows];
    for (size_t row_i = 0; row_i < totalRows; ++row_i)
    {
        for (size_t col_i = 0; col_i < totalCols; ++col_i)
        {
            if (col_i >= fsim::Floormap::minCols && col_i < fsim::Floormap::maxCols)
            {
                nodes[row_i].push_back(new fsim::Node(row_i, col_i, tileSize, totalRows, totalCols));
            }
            else
                nodes[row_i].push_back(nullptr);
        }
    }

    std::vector<std::shared_ptr<fsim::Floormap>> FloorMapObjects 
    {
        std::make_shared<fsim::Floormap>(400, "floordata/ground-level.map", &window, FloorLabel::GROUND, nodes),
        std::make_shared<fsim::Floormap>(400, "floordata/2nd-level.map", &window, FloorLabel::SECOND, nodes),
        std::make_shared<fsim::Floormap>(400, "floordata/3rd-level.map", &window, FloorLabel::THIRD, nodes),
        std::make_shared<fsim::Floormap>(400, "floordata/4th-level.map", &window, FloorLabel::FOURTH, nodes)
    };

    map = FloorMapObjects[currentEnumFloor];
    map->copy_node_data_to_node_pointers();
    loadMapTexture(*map, currentEnumFloor);
    fsim::units::changeMagnitudes(currentEnumFloor);

    sf::Texture fireIconTexture;
    fireIconTexture.loadFromFile("resource/FireIcon.png");
    fireIconTexture.setSmooth(true);

    sf::Clock deltaClock;
    
    while (window.isOpen())
    {
        window.setView(map->mapView);
        sf::Vector2f worldPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::Event event;
        while(window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (event.type == sf::Event::MouseWheelMoved)
            {
                if (modalModify == modifyPointState::NONE)
                    fsim::Controller::zoomEvent(event.mouseWheel.delta, map->mapView, &window, map->mouseValue);
            }
        }
            ImGui::SFML::Update(window, deltaClock.restart());

            window.clear(sf::Color::White);

            map->drawMap(&window);

            window.draw(*map->nodePositions);

            for (auto startNode : map->startingPoints)
            {
                if (startNode->node != nullptr)
                    window.draw(startNode->point);
            }

            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::Begin("MU Fire Escape Simulator");
            if (ImGui::CollapsingHeader("What is this?"))
            {

            }

            if (ImGui::CollapsingHeader("Help"))
            {
            }

            if (ImGui::CollapsingHeader("Simulator"))
            {
                if (startingPointsChanged)
                {
                    strcpy(startingPointsCString, (std::string("Starting Points: ") + std::string(std::to_string(map->startingPoints.size()))).c_str());
                    startingPointsChanged = false;
                }

                if (floorChanged)
                {
                    strcpy(floorCString, (std::string("Floor Level: ") + std::string(std::to_string(currentLevel.first)) + " " + std::string(currentLevel.second)).c_str());
                    floorChanged = false;
                }
                ImGui::Text(floorCString); ImGui::SameLine();

                if (ImGui::Button("Up"))
                {
                    if ((FloorLabel)(currentEnumFloor + 1) != FloorLabel::GROUND && 
                        (FloorLabel)(currentEnumFloor + 1) != FloorLabel::SECOND &&
                        (FloorLabel)(currentEnumFloor + 1) != FloorLabel::THIRD  &&
                        (FloorLabel)(currentEnumFloor + 1) != FloorLabel::FOURTH)
                    {
                        ImGui::End();
                        ImGui::SFML::Render(window);
                        continue;
                    }
                    currentLevel = FloorLevels[(FloorLabel)(currentEnumFloor + 1)];
                    currentEnumFloor = (FloorLabel)(currentEnumFloor + 1);
                    map = FloorMapObjects[currentEnumFloor];
                    map->copy_node_data_to_node_pointers();
                    loadMapTexture(*map, currentEnumFloor);
                    strcpy(startingPointsCString, (std::string("Starting Points: ") + std::string(std::to_string(map->startingPoints.size()))).c_str());
                    floorChanged = true;
                }
                ImGui::SameLine();

                if(ImGui::Button("Down"))
                {
                    if ((FloorLabel)(currentEnumFloor - 1) != FloorLabel::GROUND && 
                        (FloorLabel)(currentEnumFloor - 1) != FloorLabel::SECOND &&
                        (FloorLabel)(currentEnumFloor - 1) != FloorLabel::THIRD  &&
                        (FloorLabel)(currentEnumFloor - 1) != FloorLabel::FOURTH)
                    {
                        ImGui::End();
                        ImGui::SFML::Render(window);
                        continue;
                    }
                    currentLevel = FloorLevels[(FloorLabel)(currentEnumFloor - 1)];
                    currentEnumFloor = (FloorLabel)(currentEnumFloor - 1);
                    map = FloorMapObjects[currentEnumFloor];
                    map->copy_node_data_to_node_pointers();
                    loadMapTexture(*map, currentEnumFloor);
                    strcpy(startingPointsCString, (std::string("Starting Points: ") + std::string(std::to_string(map->startingPoints.size()))).c_str());
                    floorChanged = true; 
                }
                
                ImGui::Separator();

                if (ImGui::BeginTable("PointsTable", 2))
                {
                    ImGui::TableNextColumn(); ImGui::Text(startingPointsCString); 
                    ImGui::TableNextColumn(); 
                    std::string labelIgnitionPoint = "Ignition Points: " + std::to_string(map->fireGraphicsList.size());
                    ImGui::Text(labelIgnitionPoint.c_str());
                    ImGui::EndTable();
                }

                if (ImGui::Button("Visualize"))
                {
                    modifyNodes(*map, [](fsim::Node* node){ 
                        node->updateNeighbors(map->nodes, map->minCols, map->maxCols); 
                        node->switchColor(sf::Color(0.0f, 0.0f, 0.0f, 0.0f));
                    });

                    map->results.clear();

                    fsim::Algorithms::calculateRisk(map->nodes, map->fireGraphicsList, map->getTotalRows(), std::make_pair(map->minCols, map->maxCols));
                    for (auto startNode : map->startingPoints)
                    {
                        if (startNode->node != nullptr)
                        {
                            exitsStored.clear();
                            auto previous_nodes =  fsim::Algorithms::dijkstra(startNode->node, nullptr, map->nodes, map->getTotalRows(), std::make_pair(map->minCols, map->maxCols), false, true);
                            bool exit_node_within_previous_node = false;
                            for (auto exitNode : map->exitNodes)
                            {
                                if (previous_nodes.find(exitNode) == previous_nodes.end())
                                {
                                    continue;
                                }
                                else 
                                    exit_node_within_previous_node = true;
                                    
                                uint32_t nodeCount = fsim::Algorithms::reconstruct_path(exitNode, startNode->node, previous_nodes, false).node_count;
                                exitsStored.push_back(std::make_pair(exitNode, nodeCount));
                            }
                            
                            fsim::Results results;

                            if (!exit_node_within_previous_node)
                            {
                                exitsStored.clear();
                                auto previous_nodes = fsim::Algorithms::dijkstra(startNode->node, nullptr, map->nodes, map->getTotalRows(), std::make_pair(map->minCols, map->maxCols), false, false);

                                for (auto exitNode : map->exitNodes)
                                {
                                    if (previous_nodes.find(exitNode) == previous_nodes.end())
                                        continue;
                                    
                                    uint32_t nodeCount = fsim::Algorithms::reconstruct_path(exitNode, startNode->node, previous_nodes, false).node_count;
                                    exitsStored.push_back(std::make_pair(exitNode,  nodeCount));
                                }
                                auto minExitNode = *std::min_element(exitsStored.begin(), exitsStored.end(), [](auto &left, auto &right) {
                                                    return left.second < right.second;});
                                // boxCountDistance = (fsim::Algorithms::reconstruct_path(minExitNode.first, startNode->node, previous_nodes, true).node_count - 1) * 0.5544342178f;
                                results = (fsim::Algorithms::reconstruct_path(minExitNode.first, startNode->node, previous_nodes, true));
                            }
                            else 
                            {
                                auto minExitNode = *std::min_element(exitsStored.begin(), exitsStored.end(), [](auto &left, auto &right) {
                                                    return left.second < right.second;});
                                // boxCountDistance = (fsim::Algorithms::reconstruct_path(minExitNode.first, startNode->node, previous_nodes, true)).node_count;
                                results = (fsim::Algorithms::reconstruct_path(minExitNode.first, startNode->node, previous_nodes, true));
                                // boxCountDistance = (boxCountDistance != FSIM_NOPATHDIST) ? (boxCountDistance - 1) * 0.5544342178f : boxCountDistance;
                            }

                            // std::cout << boxCountDistance << std::endl;
                            std::cout << "Distance traveled: " << results.distance_traveled << std::endl;
                            std::cout << "Danger Indicator: " << results.danger_indicator_average << std::endl;
                            std::cout << "Safe path proportion: " << results.safe_path_proportion << std::endl;
                            std::cout << "Risky path proprtion: " << results.risky_path_proportion << std::endl << std::endl;

                            map->results.push_back(results);
                            map->results[map->results.size() - 1].point_name = startNode->buffer;
                            // std::cout << "Obs: " << results.obstructions_count << " " << "node_counts: " << results.node_count << std::endl;

                        }

                    }


                    map->initVertexArray();
                }
                if (ImGui::CollapsingHeader("View Controls"))
                {
                    if (ImGui::BeginTable("split", 2))
                    {
                        ImGui::TableNextColumn(); ImGui::Checkbox("WASD Movement", &enableWASD); 
                        ImGui::TableNextColumn(); 

                        ImGui::RadioButton("Navigate", &screenClickHandle, 0);
 
                        ImGui::EndTable();
                    }
                    ImGui::Separator();

                    float ratio = ((float)map->mouseValue) / (fsim::Controller::zoomValues.size() - 1.0f);
                    int percent = ratio * 100.0f;

                    std::string zoomPercentage = "Zoom: " + std::to_string(percent) + " %%";
                    ImGui::Text(zoomPercentage.c_str()); ImGui::SameLine();
                    if (ImGui::Button("+"))
                    {
                        if (map->mouseValue < fsim::Controller::zoomValues.size() - 1)
                        {
                            fsim::Controller::zoomEvent(1, map->mapView, &window, map->mouseValue);
                        }
                    }
                     ImGui::SameLine();
                    if (ImGui::Button("-"))
                    {
                        if (map->mouseValue > 0)
                        {
                            fsim::Controller::zoomEvent(-1, map->mapView, &window, map->mouseValue);
                        }
                    }
                }
                if (ImGui::CollapsingHeader("Starting Points"))
                {
                    if (ImGui::BeginTable("table1", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        // Display headers so we can inspect their interaction with borders.
                        // (Headers are not the main purpose of this section of the demo, so we are not elaborating on them too much. See other sections for details)

                        ImGui::TableSetupColumn("Starting Points");
                        ImGui::TableHeadersRow();

                        for (size_t row = 0; row < map->startingPoints.size(); row++)
                        {
                            ImGui::TableNextRow();
                            for (int column = 0; column < 1; column++)
                            {
                                ImGui::TableSetColumnIndex(column);
                                ImGui::Text(map->startingPoints[row]->buffer); ImGui::SameLine();
                                if(ImGui::Button(("Modify##" + std::to_string(row)).c_str()))
                                {
                                    startingPointTemp = map->startingPoints[row];
                                    modalModify = modifyPointState::MODIFY;
                                } ImGui::SameLine();
                                if(ImGui::Button(("Locate##" + std::to_string(row)).c_str()))
                                {
                                    if (map->startingPoints[row]->node != nullptr)
                                    {
                                        map->mapView.setCenter(sf::Vector2f(map->startingPoints[row]->node->getWorldPos().x,
                                        map->startingPoints[row]->node->getWorldPos().y));
                                        map->mouseValue = 12;
                                        map->mapView.setSize(sf::Vector2f(1366.0f * fsim::Controller::zoomValues[map->mouseValue], 
                                        768.0f * fsim::Controller::zoomValues[map->mouseValue]));
                                        window.setView(map->mapView);
                                    }
                                }
                                ImGui::SameLine();
                                if (ImGui::Button(("Move##" + std::to_string(row)).c_str()))
                                {
                                    screenClickHandle = 0;
                                    modifyNodes(*map, [](fsim::Node* node) { 
                                        // if (node->obstruction == false)
                                            node->switchColor(sf::Color(0.0f, 0.0f, 0.0f, 0.0f)); 
                                    });

                                    map->initVertexArray();
                                    sf::Color color(sf::Color(
                                        map->startingPoints[row]->point_rgba.x * 255.0f,
                                        map->startingPoints[row]->point_rgba.y * 255.0f,
                                        map->startingPoints[row]->point_rgba.z * 255.0f,
                                        map->startingPoints[row]->point_rgba.w * 255.0f)
                                    );
                                    map->startingPoints[row]->point.setFillColor(color);

                                    startPointMoving = true;
                                    startingPointTemp = map->startingPoints[row];
                                    enableWASD = true;
                                }
                                ImGui::SameLine();
                                if(ImGui::Button(("Delete##" + std::to_string(row)).c_str()))
                                {
                                    modifyNodes(*map, [](fsim::Node* node) { node->switchColor(sf::Color(0.0f, 0.0f, 0.0f, 0.0f)); });
                                    map->initVertexArray();
                                    map->startingPoints[row]->node = nullptr;
                                    delete map->startingPoints[row];
                                    map->startingPoints.erase(map->startingPoints.begin() + row);
                                    startingPointsChanged = true;
                                    break;
                                }
                                 ImGui::SameLine();
                                
                                ImGui::ColorEdit4("", (float*)&(map->startingPoints[row]->point_rgba), ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs);
                                // ImGui::ColorPicker4("My Color", (float*)&(startingPoints[row]->point_rgba), misc_flags, NULL);
                                // else if (contents_type)
                                //     ImGui::Button(buf, ImVec2(-FLT_MIN, 0.0f));
                            }
                        }

                        ImGui::EndTable();
                    }
                    if (ImGui::Button("Add"))
                    {
                        modalModify = modifyPointState::CREATE;
                    }

                }
                if (ImGui::CollapsingHeader("Fire Sources"))
                {
                    if (ImGui::BeginTable("CreateFireNodeTables", 2))
                    {
                        ImGui::TableNextColumn();
                        ImGui::RadioButton("Add manually", &screenClickHandle, 1);
                        ImGui::TableNextColumn(); 
                        if (ImGui::Button("Auto generate"))
                        {

                        }
                        ImGui::EndTable();
                    }

                    ImGui::PushItemWidth(150.0f);
                    ImGui::SliderFloat("Heat Flux (kW/m2) ", &heatFluxValue, 0.0f, 300.0f);
                    ImGui::Separator();
                    if (ImGui::BeginTable("table1", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Fire Points Table");
                        ImGui::TableHeadersRow();

                        for (size_t row = 0; row < map->fireGraphicsList.size(); ++row)
                        {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            std::stringstream ssHeatFlux;
                            ssHeatFlux << std::setprecision(5) << map->fireGraphicsList[row].heatFluxValue << " kW/m2";
                            ImGui::Text(ssHeatFlux.str().c_str());
                            ImGui::SameLine();
                            if(ImGui::Button(("Locate##" + std::to_string(row)).c_str()))
                            {
                                if (map->fireGraphicsList[row].node != nullptr)
                                {
                                    map->mapView.setCenter(sf::Vector2f(map->fireGraphicsList[row].node->getWorldPos().x,
                                    map->fireGraphicsList[row].node->getWorldPos().y));
                                    map->mouseValue = 12;
                                    map->mapView.setSize(sf::Vector2f(1366.0f * fsim::Controller::zoomValues[map->mouseValue], 
                                    768.0f * fsim::Controller::zoomValues[map->mouseValue]));
                                    window.setView(map->mapView);
                                }
                            }
                            ImGui::SameLine();
                            if(ImGui::Button(("Modify##" + std::to_string(row)).c_str()))
                            {
                            }
                            ImGui::SameLine();
                            if(ImGui::Button(("Delete##" + std::to_string(row)).c_str()))
                            {
                                if (map->fireGraphicsList[row].node != nullptr)
                                {
                                    fsim::Node* selectedNode = map->fireGraphicsList[row].node;
                                    map->fireGraphicsList.erase(map->fireGraphicsList.begin() + row);
                                    for (size_t i = 0; i < map->getTotalRows(); ++i)
                                    {
                                        for (size_t k = map->minCols; k < map->maxCols; ++k)
                                        {
                                            if (map->nodes[i][k] != selectedNode)
                                            {
                                                float distance = std::sqrt(std::pow((float)selectedNode->col - (float)map->nodes[i][k]->col, (float)2.0f) + std::pow((float)selectedNode->row - (float)map->nodes[i][k]->row, (float)2.0f));
                                                if (distance * fsim::units::UNIT_DISTANCE <= fsim::units::STANDARD_HEAT_FLUX_RADIUS)
                                                {
                                                    // map->nodes[i][k]->switchColor(sf::Color(255,115, 0, 90.0f));
                                                    map->nodes[i][k]->obstruction = false;
                                                    
                                                }
                                            }

                                        }
                                    }
                                    map->initVertexArray();
                                }
                            }
                        }

                        ImGui::EndTable();
                    }

                }
                if (ImGui::CollapsingHeader("Results"))
                {
                    if (ImGui::BeginTable("Resultstable1", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Point");
                        ImGui::TableSetupColumn("D");
                        ImGui::TableSetupColumn("I");
                        ImGui::TableSetupColumn("Ps");
                        ImGui::TableSetupColumn("Pd");
                        ImGui::TableHeadersRow();

                        std::stringstream ss;

                        for (size_t row = 0; row < map->results.size(); ++row)
                        {
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text(map->results[row].point_name.c_str());

                            ImGui::TableSetColumnIndex(1);
                            ss << std::setprecision(4) << map->results[row].distance_traveled << " m";
                            ImGui::Text(ss.str().c_str());
                            ss.str("");

                            ImGui::TableSetColumnIndex(2);
                            ss << std::setprecision(4) << map->results[row].danger_indicator_average;
                            ImGui::Text(ss.str().c_str());
                            ss.str("");

                            ImGui::TableSetColumnIndex(3);
                            ss << std::setprecision(4) << map->results[row].safe_path_proportion;
                            ImGui::Text(ss.str().c_str());
                            ss.str("");

                            ImGui::TableSetColumnIndex(4);
                            ss << std::setprecision(4) << map->results[row].risky_path_proportion;
                            ImGui::Text(ss.str().c_str());
                            ss.str("");
                            
                        }

                        ImGui::EndTable();
                    }
                }

            }

            // if (ImGui::CollapsingHeader("Results"))
            // {
            // }
            
            if (modalModify == modifyPointState::CREATE)
                displayModifyModal(modifyPointState::CREATE);

            else if (modalModify == modifyPointState::MODIFY)
            {
                displayModifyModal(modifyPointState::MODIFY);
            }

            ImGui::End();

            ImGui::SFML::Render(window);

            if (startPointMoving)
            {
                startingPointTemp->point.setPosition(sf::Vector2f(worldPos.x, worldPos.y));
                window.draw(startingPointTemp->point);

                if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
                {
                    if (!mouseDown){
                        sf::Vector2u position = map->clickPosition(worldPos);
                        fsim::Node* selectedNode = map->nodes[position.x][position.y];
                        fsim::Node* calculatedSelectedNode = fsim::Algorithms::bfsGetNearestStart(selectedNode, map->nodes, map->getTotalRows(), map->getTotalCols());
                        startingPointTemp->node = calculatedSelectedNode;

                        startingPointTemp->point.setPosition(sf::Vector2f(startingPointTemp->node->getWorldPos().x + (startingPointTemp->node->getTileSize()/2.0f), 
                        startingPointTemp->node->getWorldPos().y + (startingPointTemp->node->getTileSize()/2.0f)));

                        startPointMoving = false;
                        startingPointTemp = nullptr;
                        mouseDown = true;
                    } 
                }
                else mouseDown = false;
            }

            window.display();

            if (mouseOnImGui && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                mouseOnImGui = false;

            if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
            {
                if (!ImGui::IsAnyItemHovered() && !mouseOnImGui){
                    // * Backends *//
                    if (enableWASD)
                    {
                        fsim::Controller::keyboardEvent(map->mapView, &window);
                    }


                    if (screenClickHandle == 0)
                        fsim::Controller::dragEvent(map->mapView, &window);

                    else if (screenClickHandle == 1)
                    {
                        if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
                        {
                            if (!mouseDown)
                            {
                                modifyNodes(*map, [](fsim::Node* node) { node->switchColor(sf::Color(0.0f, 0.0f, 0.0f, 0.0f)); });
                                sf::Vector2u position = map->clickPosition(worldPos);
                                fsim::Node* selectedNode = map->nodes[position.x][position.y];
                                // selectedNode->switchColor(sf::Color(255, 70, 0, 255.0f));
                                map->generateFireGraphics(selectedNode, &fireIconTexture, heatFluxValue);
                                for (size_t i = 0; i < map->getTotalRows(); ++i)
                                {
                                    for (size_t k = map->minCols; k < map->maxCols; ++k)
                                    {
                                        if (map->nodes[i][k] != selectedNode)
                                        {
                                            float distance = std::sqrt(std::pow((float)selectedNode->col - (float)map->nodes[i][k]->col, (float)2.0f) + std::pow((float)selectedNode->row - (float)map->nodes[i][k]->row, (float)2.0f));
                                            if (distance * fsim::units::UNIT_DISTANCE < fsim::units::STANDARD_HEAT_FLUX_RADIUS)
                                            {
                                                // map->nodes[i][k]->switchColor(sf::Color(255,115, 0, 90.0f));
                                                map->nodes[i][k]->obstruction = true;
                                                
                                            }
                                        }

                                    }
                                }
                                map->initVertexArray();
                                mouseDown = true;
                            }
                        } else mouseDown = false;
                    }
                }
                else
                {
                    if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
                    {
                        mouseOnImGui = true;
                    }
                }
            }

            if (sf::Mouse::isButtonPressed(sf::Mouse::Middle))
                std::cout << "FUCKER: " << map->results[0].point_name.c_str() << std::endl;

            // if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            // {
            //     // for (size_t i = 0; i < totalRows; ++i)
            //     // {
            //     //     for (size_t k = 37; k < 359; ++k)
            //     //     {
            //     //         if (map->nodes[i][k]->type == fsim::NODETYPE::DefaultPath)
            //     //             map->nodes[i][k]->switchColor(sf::Color::Blue);

            //     //     }
            //     // }
            //     // map->initVertexArray();
            //     if (!mouseDown)
            //     {
            //         sf::Vector2u position = map->clickPosition(worldPos);
            //         fsim::Node* selectedNode = map->nodes[position.x][position.y];
            //         std::cout << selectedNode->riskValue << std::endl;
            //         mouseDown = true;
            //     }
            // } else mouseDown = false;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();
        
    }
    ImGui::SFML::Shutdown();
    return 0;
}