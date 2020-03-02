#include "chrono/physics/ChSystemNSC.h"
#include "chrono_irrlicht/ChIrrApp.h"
#include "chrono\fea\ChNodeFEAxyz.h"
#include "chrono\fea\ChLinkPointFrame.h"
#include "chrono\fea\ChElementTetra_4.h"
#include "chrono\physics\ChBodyEasy.h"
#include "fea/ChVisualizationFEAmesh.h"

void SetFixedBase(const std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>& baseNodes, 
	std::vector<std::shared_ptr<chrono::ChPhysicsItem>>& physicsItems, 
	std::shared_ptr<chrono::ChBodyEasyBox>& wall)
{
	/*
		This function is used to fix the base nodes of the beam in the wall
		Parameters:
			-baseNoses is a vector of pointers to a FEA 3D node
			-wall is the ChBodyEasyBox you created earlier 
			-physicalItems is a list of objects that need to be rendered

	=======================================================================================================
		Here you will have to link the nodes from the base of the beam with the rigid body (ChBody) you created earlier 
			To do that you will have to use ChLinkPointFrame , a class used to constrain ChNoseFEAxyz nodes to ChBody objects :
				-This class has a function called Initialize with takes a FEA node and a ChBody
			Don't forget to add the constrainit in the physical item

		Code ~ 3 lines 

	*/
	for (const auto& node : baseNodes)
	{
		auto constraint = std::make_shared<chrono::fea::ChLinkPointFrame>();
		constraint->Initialize(node, wall);
		physicsItems.emplace_back(constraint);
	}
}

void BuildBlock(const std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>& nodes, 
	std::shared_ptr<chrono::fea::ChMesh>& mesh, 
	std::shared_ptr<chrono::fea::ChContinuumElastic>& material)
{
	/*
		This function is responsable for building blocks(cubes) out of tetrahedrons .
		Parameters:
			- nodes  - vector of FEA nodes used to build tetrahedrons
			- mesh   - the beam mesh wich will contain tetrahedrons
			- material - the material used for tetrahedrons
	*/
	auto BuildTetra = [&](const auto& firstNode, const auto& secondNode, const auto& thirdNode, const auto& fourthNode)
	{
		/*
			This lambda function will actually create the tetrahedrons
				Parameters :
					4 nodes representing the vertexes of a thetrahedron
			To achieve this task you will have to use chrono::fea::ChElementTetra_4 , set it's nodes and it's material and  add it to the mesh
			Usefull functions :
				-for tetra : SetNodes(node1,node2,node3,node4) , SetMaterial
				-for mesh : AddElement

		Code ~ 4 lines
		*/
		auto tetra = std::make_shared<chrono::fea::ChElementTetra_4>(); // Creem un obiect de tip tetraedru
		tetra->SetNodes(firstNode, secondNode, thirdNode, fourthNode); // Setam nodurile tetraedrului 
		tetra->SetMaterial(material);
		mesh->AddElement(tetra);
	};

	/*
		This is the part that creates the block .
		The min number of tetrahedrons that can fit in a block is  5 that is why we call the BuildTetra 5 times
		Check this link to get a better view 
		https://engineering.stackexchange.com/questions/2949/minimum-number-of-tetra-elements-required-to-represent-a-cube or https://www.ics.uci.edu/~eppstein/projects/tetra/
	*/
	BuildTetra(nodes[4], nodes[1], nodes[2], nodes[0]);
	BuildTetra(nodes[1], nodes[2], nodes[4], nodes[7]);
	BuildTetra(nodes[6], nodes[7], nodes[4], nodes[2]);
	BuildTetra(nodes[1], nodes[4], nodes[5], nodes[7]);
	BuildTetra(nodes[1], nodes[2], nodes[3], nodes[7]);
}

std::vector<std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>> CreateNodes(
	std::shared_ptr<chrono::fea::ChMesh>& mesh, 
	chrono::Vector& blockSize, 
	chrono::ChVector<int>& size, std::vector<std::shared_ptr<chrono::ChPhysicsItem>>& physicsItems,
	std::shared_ptr<chrono::ChBodyEasyBox>& wall)
{
	/*
		This function will create the nodes for your beam mesh
		Parameters:
			-mesh  = the mesh in wich you will add the nodes
			-block size = a vector that will contain the distance between nodes on axis X,Y and Z
			-size = a vector that will contain the number of nodes for your mesh on axis X,Y and Z
			-physicalItems = a list of objects that need to be rendered
			-wall = the ChBodyEasyBox you created earlier 

	*/
	try
	{
		/* We need to put a "+1" because we want to have the number of nodes, not cubes */
		int length = size.x() + 1, height = size.y() + 1, width = size.z() + 1;
		double xSize = blockSize.x(), ySize = blockSize.y(), zSize = blockSize.z();
		std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>> baseNodes;

		/* height x width x length matrix of nodes  */
		std::vector<std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>> nodes
		(height, std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>
			(width, std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>(length)));

		for (size_t levelIndex = 0; levelIndex < height; ++levelIndex)
		{
			for (size_t lineIndex = 0; lineIndex < width; ++lineIndex)
			{
				for (size_t columnIndex = 0; columnIndex < length; ++columnIndex)
				{
					/* We create the nodes at the given position in space */
					auto node = std::make_shared<chrono::fea::ChNodeFEAxyz>(
						chrono::Vector(lineIndex * xSize, levelIndex * ySize, columnIndex * zSize)
						);
					nodes[levelIndex][lineIndex][columnIndex] = node;
					mesh->AddNode(node);

					/* We select the nodes that represent the base of the beam */
					if (lineIndex == 0)
					{
						baseNodes.emplace_back(node);
					}
				}
			}
		}
		std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyz>(mesh->GetNodes().back())->SetForce(chrono::Vector(0.0, -10000.0, 0.0));
		std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyz>(mesh->GetNodes().back())->SetMass(100.0);
		
		/* Function that needs to be implemented */
		SetFixedBase(baseNodes, physicsItems, wall);
		return nodes;
	}
	catch (const std::exception & exception)
	{
		std::cout << exception.what() << std::endl;
		return std::vector<std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>>();
	}
}

int main(int argc, char* argv[])
{
	/* System and Application setup */
	chrono::SetChronoDataPath(CHRONO_DATA_DIR);

	/*
	 _     _         __   __            _______  _                            (_)
	(_)   (_)  ____ (__) (__)          (__ _ __)(_)      ____  _      ____    (_)
	(_)___(_) (____) (_)  (_)  ___        (_)   (_)__   (____)(_)__  (____)   (_)
	(_______)(_)_(_) (_)  (_) (___)       (_)   (____) (_)_(_)(____)(_)_(_)   (_)
	(_)   (_)(__)__  (_)  (_)(_)_(_)      (_)   (_) (_)(__)__ (_)   (__)__     _
	(_)   (_) (____)(___)(___)(___)       (_)   (_) (_) (____)(_)    (____)   (_)	                                                                           _


		This is a tutorial that will take you through the basics of project chrono .
		By the end of this tutorial you will be able you create your own space with different objects .
		Reqirements :
			- you should have a good understanding of smart pointers since you will use them alot (especialy shared_ptr)
			please check this link before starting : https://docs.microsoft.com/en-us/cpp/cpp/smart-pointers-modern-cpp?view=vs-2019

===========================================================================================================================================================================================

	First thing you will need to do is to create a system in wich your objects will exist :
		-In order to achive this thing you will need to use chrono::ChSystemNSC
			Th : This class is used to reprezent a multibody physical system , it also has some global settings like gravity acceleration,
			as a final purpose the system is responsable of performing the entire  physical simulation
			NSC stands for Non-smooth Complementary-based

	Code ~ 1 line
	*/

	auto system = std::make_shared<chrono::ChSystemNSC>();

	/*
		On this section you will need to creat the GUI and luckily for you this is preaty simple to do :
			-In order to achive this you will need to use chrono::irrlicht::ChIrrApp
				Th : This class has the following arguments :
						-ChSystem *  - a pointer to the system that you want to render
						- const w_chart - a string representing the title of the app
						-irr::core::dimesnion2d<irr::u32>(width,height) - the dimensions of the window
						-others that can be left with the default values
				The Class will be able to render the objects and the events that happen in the system
				We already set the skybox and Lights for you ,but  is up to you to position the camera :
					-AddTipicalCamera(irr::core::vector3df(X,Y,Z))

	Code ~ 2 line
	*/

	auto application = std::make_shared<chrono::irrlicht::ChIrrApp>(system.get(), L"Environment", irr::core::dimension2d<irr::u32>(1200, 720), false);
	application->AddTypicalSky();
	application->AddTypicalLights();
	application->AddTypicalCamera(irr::core::vector3df(3, 3, 3));

	/*
		Next thing you will need to add a wall in your system :
			-You can create a rigid body using the class ChBody and adding it into your system but doing it like this  won't  be visible
			In order to have a visible wall in your application you will have to use ChBodyEasyBox :
				-It's a class that is derived from ChBody but it also has visual assets
				Consturctor of this class has the following paremeters :
					- double Xsize
					- double Ysize
					- double Zsize
					- bool collide = a parameter that enables collision detection (make it false since we won't need collision for this tutorial)
					- bool visual asset = enables the visualisation of the body in the application
				Other functions that you might find usefull from this class are :
					-SetBodyFixed(bool) - it makes the body fixed in space ( the gravitational force won't act upon it)
					-SetPos(chrono::Vector(X,Y,Z)) - sets the positon of the object in space
			Don't forget to add you wall in the system

		Code ~ 4-5 lines
	*/

	auto wall = std::make_shared<chrono::ChBodyEasyBox>(5, 0.1, 5, 1000, false, true);
	wall->SetBodyFixed(true);
	wall->SetPos(chrono::Vector(0, 0, 0));
	wall->SetRot(Q_from_AngAxis(chrono::CH_C_PI / 2, chrono::VECT_Z));

	/*
		In this section you will start by preaparing the mesh of your beam
		What is a mesh ? :
			- a mesh is a collection of vertices,edges and faces that desctibe a 3D shape
		You can create a mesh using chrono::fea::ChMesh

	Code ~ 1 line
	*/
	auto beamMesh = std::make_shared<chrono::fea::ChMesh>();

	/*
		Just because you created a mesh it doesn't mean you will be able to render it , to do that you will need a ChVisualizationFEAmesh
		Parameters for ChvisualizationFEAmesh consturctor :
			-&ChMesh - the mesh you want to visualize
		You will have to Set the type of strain you'll want to plot :
			- check function SetFEMdataType
			* as type of strain we recomand using chrono::fea::ChVistualiztionFEAmesh::E_PLOT_ELEM_STRAIN_VONMISES
		You will have to set the color scale to be able to see the tension in your beam :
			- check function SetColorscaleMinMax

	Code ~ 3 lines

	*/

	auto visualizationMesh = std::make_shared<chrono::fea::ChVisualizationFEAmesh>(*beamMesh.get());
	visualizationMesh->SetFEMdataType(chrono::fea::ChVisualizationFEAmesh::E_PLOT_ELEM_STRAIN_VONMISES);
	visualizationMesh->SetColorscaleMinMax(-0.5, 0.5);

	/*
		Last but not least  you will have to add the visaual asset (visualizationMesh) to your beamMesh
		Check AddAsset
	*/

	beamMesh->AddAsset(visualizationMesh);

	/* List of objects that need to be rendered */
	std::vector<std::shared_ptr<chrono::ChPhysicsItem>> physicsItemsBeam;
	chrono::Vector blockSize = chrono::Vector(0.3);
	chrono::ChVector<int> size = chrono::ChVector<int>(2, 2, 7);

	physicsItemsBeam.emplace_back(beamMesh);

	/* The physical material of the object */
	std::shared_ptr<chrono::fea::ChContinuumElastic> material = std::make_shared<chrono::fea::ChContinuumPlasticVonMises>();
	material->Set_E(0.005e9);
	material->Set_v(0.3);

	/* The function we implement where we create all the nodes of the beam */
	auto nodes = CreateNodes(beamMesh, blockSize, size, physicsItemsBeam, wall);

	/* We iterate through all the nodes to create the respective cubes for them */
	for (size_t levelIndex = 0; levelIndex < nodes.size() - 1; ++levelIndex)
	{
		for (size_t lineIndex = 0; lineIndex < nodes.front().size() - 1; ++lineIndex)
		{
			for (size_t columnIndex = 0; columnIndex < nodes.front().front().size() - 1; ++columnIndex)
			{
				/* The function we implement where we create all the cubes from the beam */
				BuildBlock
				({
					nodes[levelIndex][lineIndex][columnIndex],
					nodes[levelIndex][(lineIndex + 1)][columnIndex],
					nodes[levelIndex][lineIndex][(columnIndex + 1)],
					nodes[levelIndex][(lineIndex + 1)][(columnIndex + 1)],

					nodes[(levelIndex + 1)][lineIndex][columnIndex],
					nodes[(levelIndex + 1)][(lineIndex + 1)][columnIndex],
					nodes[(levelIndex + 1)][lineIndex][(columnIndex + 1)],
					nodes[(levelIndex + 1)][(lineIndex + 1)][(columnIndex + 1)],
					}, beamMesh, material);
			}
		}
	}

	/* Add all physical items to your system */
	for (auto item : physicsItemsBeam)
	{
		system->Add(item);
	}
	system->Add(wall);

	/* Render setup */
	application->AssetBindAll();
	application->AssetUpdateAll();
	system->Setup();

	/*
		In the end you will need to seet a solver type to your system (SetSolverType)
		all the solver types can be found in ChSolver::Type::
		we recommand MINRES
	*/
	system->SetSolverType(chrono::ChSolver::Type::MINRES);

	/* Rendering infinite while loop */
	while (application->GetDevice()->run())
	{
		application->BeginScene();
		application->DrawAll();
		application->DoStep();
		application->EndScene();
	}

	return 0;
}
