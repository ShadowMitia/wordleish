#include <cctype>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include <array>
#include <filesystem>
#include <fstream>
#include <random>

#include <fmt/core.h>

#include <SDL2/SDL.h>
#include <SDL_opengl.h>

#include <imgui.h>
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(token);
   }
   return tokens;
}

std::string read_text_file(std::filesystem::path const& path) {
  const auto size = std::filesystem::file_size(path);
  
  std::ifstream file(path);

  std::string result;
  result.resize(size);

  if (not file.read(result.data(), static_cast<long>(size))) {
    return {};
  }
  
  return result;
}

int main() {
  const auto wordle_answers = []() {
      const auto wordle_answers_raw = read_text_file("assets/wordle-answers-alphabetical.txt");
    return split(wordle_answers_raw, '\n');
  }();
  
  const auto wordle_guesses = []() {
    const auto wordle_guesses_raw = read_text_file("assets/wordle-allowed-guesses.txt");
    auto splitted = split(wordle_guesses_raw, '\n');
    std::sort(splitted.begin(), splitted.end());
    return splitted;
  }();

  std::random_device rd;
  std::minstd_rand rand(rd());
  std::uniform_int_distribution<std::size_t> random_answer(0, wordle_answers.size());
  
  
  fmt::print("Wordle!\n");

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fmt::print("[ERROR] Couldn't load SDL : {}\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 23);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  const auto window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window* window = SDL_CreateWindow("Wordle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable VSYNC

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  io.Fonts->AddFontFromFileTTF("assets/liberation_font/LiberationSans-Bold.ttf", 16);
  
  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  const char* glsl_version = "#version 450";
  ImGui_ImplOpenGL3_Init(glsl_version);

  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  std::string word_to_guess = wordle_answers[random_answer(rand)];

  enum class Status  {
    UNKNOWN,
    VALID,
    WRONG_POSITION
  };

  std::size_t max_tries = 6;
  std::size_t tries = 1;
  
  std::vector<std::vector<std::array<char, 2>>> inputs(max_tries);
  for (auto& v : inputs) {
    v = std::vector<std::array<char, 2>>(word_to_guess.size());
  }

  std::vector<std::vector<Status>> status(max_tries);
  for (auto& v : status) {
    v = std::vector<Status>(word_to_guess.size(), Status::UNKNOWN);
  }

  constexpr auto green = IM_COL32(0, 255, 0, 255);
  constexpr auto yellow = IM_COL32(200, 155, 0, 255);
  constexpr auto black = IM_COL32(0, 0, 0, 255);

  std::size_t current_cell = 0;
  
  bool wait_to_continue = false;
  
  bool is_looping = true;
  while (is_looping) { // WEEEEEE

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      switch (event.type) {
      case SDL_QUIT:
	is_looping = false;
	break;
      case SDL_KEYDOWN:
	if (wait_to_continue) {
	  wait_to_continue = false;
	  word_to_guess = wordle_answers[random_answer(rand)];

	  tries = 1;
	  current_cell = 0;

	  inputs = std::vector<std::vector<std::array<char, 2>>>(max_tries);
	  for (auto& v : inputs) {
	    v = std::vector<std::array<char, 2>>(word_to_guess.size());
	  }

	  status = std::vector<std::vector<Status>>(max_tries);
	  for (auto& v : status) {
	    v = std::vector<Status>(word_to_guess.size(), Status::UNKNOWN);
	  }
	  continue;
	}
	
	switch (event.key.keysym.sym) {
	case SDLK_BACKSPACE:
	  int curr = current_cell - 1;
	  if (curr >= 0) {
	    current_cell = curr;
	  }
	  inputs[tries-1][current_cell] = std::array<char, 2>();
	  break;
	}
	break;
      }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
  
    ImGui::Begin("Wordle", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

#ifdef IMGUI_HAS_VIEWPORT
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
#else 
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif

    ImGui::SetWindowFontScale(4.f);
    
    const auto style = ImGui::GetStyle();

    const auto box_width = ImGui::GetFontSize();

    const auto box_line_width = style.CellPadding.x * static_cast<float>(word_to_guess.size()) + box_width * static_cast<float>(word_to_guess.size());
   
    const auto center_padding = (io.DisplaySize.x - box_line_width) / 2.f;

    ImGui::Indent(center_padding);
    
    for (std::size_t j = 0; j < tries; j++) {
      for (std::size_t i = 0; i < word_to_guess.size(); i++) {
	if (current_cell == i) {
	  ImGui::SetKeyboardFocusHere();
	}

	ImGui::SetNextItemWidth(box_width);
	
	switch (status[j][i]) {
	case Status::VALID:
	  ImGui::PushStyleColor(ImGuiCol_FrameBg, green);
	  ImGui::PushStyleColor(ImGuiCol_Text, black);
	  break;
	case Status::WRONG_POSITION:
	  ImGui::PushStyleColor(ImGuiCol_FrameBg, yellow);
	  ImGui::PushStyleColor(ImGuiCol_Text, black);
	  break;
	default:
	  ImGui::PushStyleColor(ImGuiCol_FrameBg, black);
	  break;
	}
	
        int flags = ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_CharsUppercase;
	if (j < tries - 1) {
	  flags |= ImGuiInputTextFlags_ReadOnly;
	}	
	const auto label = fmt::format("##foo{}{}", i, j);
	ImGui::InputText(label.c_str(), inputs[j][i].data(), inputs.size(), flags);       
	
	if (ImGui::IsItemFocused() && ImGui::IsItemEdited()) {
	  current_cell++;
	  if (current_cell >= word_to_guess.size()) {
	    
	    for (std::size_t i = 0; i < word_to_guess.size(); i++) {
	      const auto character = static_cast<char>(std::tolower(inputs[j][i][0]));	     
	      if (character == word_to_guess[i]) {
		status[j][i] = Status::VALID;
	      } else if (word_to_guess.find(character) != std::string::npos) {
		status[j][i] = Status::WRONG_POSITION;
	      }
	    }

	    auto counter_valid = 0;
	    for (auto const& s : status[j]) {
	      if (s == Status::VALID) {
		counter_valid++;
	      }
	    }
	    
	    if (counter_valid == word_to_guess.size()) {
	      fmt::print("You win!\n");
	      wait_to_continue = true;
	    } else if (j >= max_tries - 1) {
	      fmt::print("You lose\n");
	      wait_to_continue = true;
	    } else {	      	    
	      tries++;
	      current_cell = 0;
	    }
	    
	  }
	}

	ImGui::PopStyleColor();
	
	ImGui::SameLine();
      }
      ImGui::NewLine();      
    }
    ImGui::End();

    ImGui::Render();
    glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();
  

  SDL_Quit();
  
  return EXIT_SUCCESS;
}
