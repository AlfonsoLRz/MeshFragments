#pragma once

#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"

#include "Geometry/Animation/CatmullRom.h"
#include "Graphics/Application/CADScene.h"
#include "Graphics/Application/Renderer.h"
#include "Graphics/Application/RenderingParameters.h"
#include "Graphics/Core/FragmentationProcedure.h"
#include "imgizmo/ImGuizmo.h"
#include "Utilities/Singleton.h"

typedef std::vector<Model3D::ModelComponent*> ModelComponentBuffer;

/**
*	@file GUI.h
*	@authors Alfonso L�pez Ruiz (alr00048@red.ujaen.es)
*	@date 08/05/2019
*/

/**
*	@brief Wrapper for GUI graphics which allows the user to interact with the scene.
*/
class GUI: public Singleton<GUI>
{
	friend class Singleton<GUI>;

protected:
	FractureParameters*				_fractureParameters;				//!< Fracture parameters
	FragmentMetadataBuffer			_fragmentMetadata;					//!< Metadata of the current fragmentation procedure
	std::vector<Model3D*>			_fragment;							//!< Model loaded from a file
	ModelComponentBuffer			_modelComponents;					//!< Model component active on the scene
	Renderer*						_renderer;							//!< Access to current scene
	RenderingParameters*			_renderingParams;					//!< Reference to rendering parameters
	CADScene*						_scene;

	// GUI state
	bool							_showAboutUs;						//!< About us window
	bool							_showControls;						//!< Shows application controls
	bool							_showFractureSettings;				//!< Displays the settings of the fragmentation process
	bool							_showFragmentList;					//!< Shows a list with the metadata of the last generated fragments//
	bool							_showFileDialog;					//!< Shows a file dialog that allows opening a point cloud in .ply format
	bool							_showRenderingSettings;				//!< Displays a window which allows the user to modify the rendering parameters
	bool							_showSceneSettings;					//!< Displays a window with all the model components and their variables
	bool							_showScreenshotSettings;			//!< Shows a window which allows to take an screenshot at any size

	std::string						_fractureText;						//!<
	std::string						_modelFilePath;						//!<

protected:
	/**
	*	@brief Constructor of GUI context provided by a graphics library (Dear ImGui).
	*/
	GUI();
	
	/**
	*	@brief Creates the navbar.
	*/
	void createMenu();

	/**
	*	@brief Calls ImGui::Spacing() for n times in a clean way.
	*/
	static void leaveSpace(const unsigned numSlots);

	/**
	*	@brief  
	*/
	void loadFonts();

	/**
	*	@brief  
	*/
	void loadStyle();

	/**
	*	@brief Renders a help icon with a message.
	*/
	static void renderHelpMarker(const char* message);

	/**
	*	@brief Renders ImGui text with two concatenated strings.
	*/
	static void renderText(const std::string& title, const std::string& content, char delimiter = ':');

	/**
	*	@brief Shows a window where some information about the project is displayed.
	*/
	void showAboutUsWindow();

	/**
	*	@brief Displays a table with the application controls (mouse, keyboard, etc).
	*/
	void showControls();

	/**
	*	@brief Displays a file dialog to open a new point cloud (.ply).
	*/
	void showFileDialog();

	/**
	*	@brief Shows a list displaying the metadata of the last generated fragments.
	*/
	void showFractureList();

	/**
	*	@brief Shows settings for fracturing the currently displayed models. 
	*/
	void showFractureSettings();

	/**
	*	@brief Shows a window with general rendering configuration.
	*/
	void showRenderingSettings();

	/**
	*	@brief Shows a list of models in the scene which can be modified.
	*/
	void showSceneSettings();

	/**
	*	@brief Shows a window with framebuffer and screenshot configuration.
	*/
	void showScreenshotSettings();

public:
	/**
	*	@brief Destructor.
	*/
	virtual ~GUI();

	/**
	*	@brief Initializes the context provided by an interface library.
	*	@param window Window provided by GLFW.
	*	@param openGLMinorVersion Minor version of OpenGL 4.
	*/
	void initialize(GLFWwindow* window, const int openGLMinorVersion);

	/**
	*	@brief Renders the interface components.
	*/
	void render();

	// ------- Getters --------

	/**
	*	@return  
	*/
	bool isMouseActive() { return ImGui::GetIO().WantCaptureMouse; }
};

