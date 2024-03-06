#include "window_warping.h"

#include <ImGuiFileDialog.h>

#include <iostream>

namespace USTC_CG
{
ImageWarping::ImageWarping(const std::string& window_name) : Window(window_name)
{
    std::string filePathName = "C:\\Users\\Admin\\Documents\\GitHub\\USTC_CG_24\\Homeworks\\2_image_warping\\data\\test.png";
    std::string label = filePathName;
    p_image_ = std::make_shared<CompWarping>(label, filePathName);
    p_image_->add_control_points(0, 0, 0, 0);
    p_image_->add_control_points(255, 0, 255, 0);
    p_image_->add_control_points(0, 255, 0, 255);
    p_image_->add_control_points(255, 255, 255, 255);
    p_image_->add_control_points(64, 32, 32, 64);
    p_image_->add_control_points(255 - 32, 64, 255 - 64, 32);
    p_image_->add_control_points(32, 255 - 64, 64, 255 - 32);
    p_image_->add_control_points(255 - 64, 255 - 32, 255 - 32, 255 - 64);
    p_image_->enable_selecting(true);
}
ImageWarping::~ImageWarping()
{
}
void ImageWarping::draw()
{
    draw_toolbar();
    draw_image_warping_options();
    if (flag_open_file_dialog_)
        draw_open_image_file_dialog();
    if (flag_save_file_dialog_ && p_image_)
        draw_save_image_file_dialog();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    if (ImGui::Begin(
            "ImageEditor",
            &flag_show_main_view_,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        if (p_image_)
            draw_image();
        ImGui::End();
    }
}
void ImageWarping::draw_toolbar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Image File.."))
            {
                flag_open_file_dialog_ = true;
            }
            if (ImGui::MenuItem("Save As.."))
            {
                flag_save_file_dialog_ = true;
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Invert") && p_image_)
        {
            p_image_->invert();
        }
        if (ImGui::MenuItem("Mirror") && p_image_)
        {
            p_image_->mirror(true, false);
        }
        if (ImGui::MenuItem("GrayScale") && p_image_)
        {
            p_image_->gray_scale();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Select Points") && p_image_)
        {
            p_image_->init_selections();
            p_image_->enable_selecting(true);
        }
        if (ImGui::MenuItem("Warping") && p_image_)
        {
            p_image_->enable_selecting(false);
            p_image_->warping();
            p_image_->init_selections();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Restore") && p_image_)
        {
            p_image_->restore();
        }
        ImGui::EndMainMenuBar();
    }
}
void ImageWarping::draw_image_warping_options()
{
    ImGui::Begin("Image Warping Options");

    auto image_warping_algorithm_type = CompWarping::Warping_IDW;
    auto fill_hole_algorithm_type = CompWarping::FillHole_None;
    float idw_mu = 2;
    float rbf_r = 30;
    float rbf_mu = -1;
    int ann_n = 3;

    if (p_image_)
    {
        image_warping_algorithm_type = p_image_->get_image_warping_algorithm();
        fill_hole_algorithm_type = p_image_->get_fill_hole_algorithm();
        idw_mu = p_image_->get_idw_mu();
        rbf_r = p_image_->get_rbf_r();
        rbf_mu = p_image_->get_rbf_mu();
        ann_n = p_image_->get_ann_sample_n();
    }
    ImGui::SameLine();
    if (image_warping_algorithm_type == CompWarping::Warping_IDW)
        ImGui::BeginDisabled();
    if (ImGui::Button("IDW") && p_image_)
        p_image_->set_image_warping_algorithm(CompWarping::Warping_IDW);
    if (image_warping_algorithm_type == CompWarping::Warping_IDW)
        ImGui::EndDisabled();
    ImGui::SameLine();

    if (image_warping_algorithm_type == CompWarping::Warping_RBF)
        ImGui::BeginDisabled();
    if (ImGui::Button("RBF") && p_image_)
        p_image_->set_image_warping_algorithm(CompWarping::Warping_RBF);
    if (image_warping_algorithm_type == CompWarping::Warping_RBF)
        ImGui::EndDisabled();
    ImGui::SameLine();

    if (image_warping_algorithm_type == CompWarping::Warping_Fisheye)
        ImGui::BeginDisabled();
    if (ImGui::Button("Fisheye") && p_image_)
        p_image_->set_image_warping_algorithm(CompWarping::Warping_Fisheye);
    if (image_warping_algorithm_type == CompWarping::Warping_Fisheye)
        ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Text("Image Warping Algorithm");

    if (fill_hole_algorithm_type == CompWarping::FillHole_None)
        ImGui::BeginDisabled();
    if (ImGui::Button("None") && p_image_)
        p_image_->set_fill_hole_algorithm(CompWarping::FillHole_None);
    if (fill_hole_algorithm_type == CompWarping::FillHole_None)
        ImGui::EndDisabled();
    ImGui::SameLine();

    if (fill_hole_algorithm_type == CompWarping::FillHole_ANN)
        ImGui::BeginDisabled();
    if (ImGui::Button("ANN") && p_image_)
        p_image_->set_fill_hole_algorithm(CompWarping::FillHole_ANN);
    if (fill_hole_algorithm_type == CompWarping::FillHole_ANN)
        ImGui::EndDisabled();
    ImGui::SameLine();

    if (fill_hole_algorithm_type == CompWarping::FillHole_Reverse)
        ImGui::BeginDisabled();
    if (ImGui::Button("Reverse") && p_image_)
        p_image_->set_fill_hole_algorithm(CompWarping::FillHole_Reverse);
    if (fill_hole_algorithm_type == CompWarping::FillHole_Reverse)
        ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::Text("Fill Hole Algorithm");

    ImGui::NewLine();

    ImGui::InputFloat("IDW mu", &idw_mu, 0.05, 0.2, "%.2f");
    ImGui::InputFloat("RBF r", &rbf_r, 1, 10, "%.0f");
    ImGui::InputFloat("RBF mu", &rbf_mu, 0.05, 0.2, "%.2f");
    ImGui::InputInt("ANN Sample Number", &ann_n, 1, 2);
    
    if (ann_n < 1)
        ann_n = 1;

    ImGui::NewLine();

    bool visible = true;
    if (p_image_)
        visible = p_image_->get_control_points_visible();
    ImGui::Checkbox("Show Control Points", &visible);
    if (p_image_)
        p_image_->set_control_points_visible(visible);

    ImGui::NewLine();

    if (ImGui::Button("Warping", ImVec2(180, 48)) && p_image_)
    {
        p_image_->restore();
        p_image_->warping();
    }

    if (p_image_)
    {
        p_image_->set_idw_mu(idw_mu);
        p_image_->set_rbf_r(rbf_r);
        p_image_->set_rbf_mu(rbf_mu);
        p_image_->set_ann_sample_n(ann_n);
    }
    ImGui::End();
}
void ImageWarping::draw_image()
    {
    const auto& canvas_min = ImGui::GetCursorScreenPos();
    const auto& canvas_size = ImGui::GetContentRegionAvail();
    const auto& image_size = p_image_->get_image_size();
    // Center the image in the window
    ImVec2 pos = ImVec2(
        canvas_min.x + canvas_size.x / 2 - image_size.x / 2,
        canvas_min.y + canvas_size.y / 2 - image_size.y / 2);
    p_image_->set_position(pos);
    p_image_->draw();
}
void ImageWarping::draw_open_image_file_dialog()
{
    IGFD::FileDialogConfig config;
    config.path = DATA_PATH;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseImageOpenFileDlg", "Choose Image File", ".png,.jpg", config);
    ImVec2 main_size = ImGui::GetMainViewport()->WorkSize;
    ImVec2 dlg_size(main_size.x / 2, main_size.y / 2);
    if (ImGuiFileDialog::Instance()->Display(
            "ChooseImageOpenFileDlg", ImGuiWindowFlags_NoCollapse, dlg_size))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePathName =
                ImGuiFileDialog::Instance()->GetFilePathName();
            std::string label = filePathName;
            p_image_ = std::make_shared<CompWarping>(label, filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
        flag_open_file_dialog_ = false;
    }
}
void ImageWarping::draw_save_image_file_dialog()
{
    IGFD::FileDialogConfig config;
    config.path = DATA_PATH;
    config.flags = ImGuiFileDialogFlags_Modal;
    ImGuiFileDialog::Instance()->OpenDialog(
        "ChooseImageSaveFileDlg", "Save Image As...", ".png", config);
    ImVec2 main_size = ImGui::GetMainViewport()->WorkSize;
    ImVec2 dlg_size(main_size.x / 2, main_size.y / 2);
    if (ImGuiFileDialog::Instance()->Display(
            "ChooseImageSaveFileDlg", ImGuiWindowFlags_NoCollapse, dlg_size))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePathName =
                ImGuiFileDialog::Instance()->GetFilePathName();
            std::string label = filePathName;
            if (p_image_)
                p_image_->save_to_disk(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
        flag_save_file_dialog_ = false;
    }
}
}  // namespace USTC_CG