#include "chrono/physics/ChSystemNSC.h"
#include "chrono_irrlicht/ChIrrApp.h"
#include "chrono\fea\ChNodeFEAxyz.h"
#include "chrono\fea\ChLinkPointFrame.h"
#include "chrono\fea\ChElementTetra_4.h"
#include "chrono\physics\ChBodyEasy.h"
#include "fea/ChVisualizationFEAmesh.h"

void SetFixedBase(const std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>& baseNodes, std::vector<std::shared_ptr<chrono::ChPhysicsItem>>& physicsItems)
{
	auto base = std::make_shared<chrono::ChBody>();
	base->SetBodyFixed(true);
	physicsItems.emplace_back(base);
	for (const auto& node : baseNodes)
	{
		auto constraint = std::make_shared<chrono::fea::ChLinkPointFrame>();
		constraint->Initialize(node, base);
		physicsItems.emplace_back(constraint);
	}
}

void BuildBlock(const std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>& nodes, std::shared_ptr<chrono::fea::ChMesh>& mesh, std::shared_ptr<chrono::fea::ChContinuumElastic>& material)
{
	auto BuildTetra = [&](const auto& firstNode, const auto& secondNode, const auto& thirdNode, const auto& fourthNode)
	{
		auto tetra = std::make_shared<chrono::fea::ChElementTetra_4>();
		tetra->SetNodes(firstNode, secondNode, thirdNode, fourthNode);
		tetra->SetMaterial(material);
		mesh->AddElement(tetra);
	};

	BuildTetra(nodes[4], nodes[1], nodes[2], nodes[0]);
	BuildTetra(nodes[1], nodes[2], nodes[4], nodes[7]);
	BuildTetra(nodes[6], nodes[7], nodes[4], nodes[2]);
	BuildTetra(nodes[1], nodes[4], nodes[5], nodes[7]);
	BuildTetra(nodes[1], nodes[2], nodes[3], nodes[7]);
}

std::vector<std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>> CreateNodes(std::shared_ptr<chrono::fea::ChMesh>& mesh, chrono::Vector& blockSize, 
	chrono::ChVector<int>& size, std::vector<std::shared_ptr<chrono::ChPhysicsItem>>& physicsItems)
{
	try
	{
		int length = size.x() + 1, height = size.y() + 1, width = size.z() + 1; // There's a +1 here so that the object created to have the exact values as expected when they're put into the constructor.
		double xSize = blockSize.x(), ySize = blockSize.y(), zSize = blockSize.z();
		std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>> baseNodes;

		std::vector<std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>> nodes
		(height, std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>
			(width, std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>(length)));

		for (size_t levelIndex = 0; levelIndex < height; ++levelIndex)
		{
			for (size_t lineIndex = 0; lineIndex < width; ++lineIndex)
			{
				for (size_t columnIndex = 0; columnIndex < length; ++columnIndex)
				{
					auto node = std::make_shared<chrono::fea::ChNodeFEAxyz>(chrono::Vector(lineIndex * xSize, levelIndex * ySize, columnIndex * zSize));
					nodes[levelIndex][lineIndex][columnIndex] = node;
					mesh->AddNode(node);
					if (lineIndex == 0)
					{
						baseNodes.emplace_back(node);
					}
				}
			}
		}
		std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyz>(mesh->GetNodes().back())->SetForce(chrono::Vector(0.0, -10000.0, 0.0));
		std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyz>(mesh->GetNodes().back())->SetMass(100.0);
		SetFixedBase(baseNodes, physicsItems);
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
	//System and Application setup
	chrono::SetChronoDataPath(CHRONO_DATA_DIR);
	auto system = std::make_shared<chrono::ChSystemNSC>();
	auto application = std::make_shared<chrono::irrlicht::ChIrrApp>(system.get(), L"Environment", irr::core::dimension2d<irr::u32>(1200, 720), false);
	application->AddTypicalSky();
	application->AddTypicalLights();
	application->AddTypicalCamera(irr::core::vector3df(3, 3, 3));

	//Create wall
	std::shared_ptr<chrono::ChBodyEasyBox> wall = std::make_shared<chrono::ChBodyEasyBox>(5, 0.1, 5, 1000, false, true);
	std::vector<std::shared_ptr<chrono::ChPhysicsItem>> physicsItemsWall;
	wall->SetBodyFixed(true);
	wall->SetPos(chrono::Vector(0, 0, 0));
	wall->SetRot(Q_from_AngAxis(chrono::CH_C_PI / 2, chrono::VECT_Z));
	system->Add(wall);

	//Create beam
	std::shared_ptr<chrono::fea::ChMesh> beamMesh = std::make_shared<chrono::fea::ChMesh>();
	std::vector<std::shared_ptr<chrono::ChPhysicsItem>> physicsItemsBeam;
	chrono::Vector blockSize = chrono::Vector(0.3);
	chrono::ChVector<int> size = chrono::ChVector<int>(2, 2, 7);

	physicsItemsBeam.emplace_back(beamMesh);

	std::shared_ptr<chrono::fea::ChContinuumElastic> material = std::make_shared<chrono::fea::ChContinuumPlasticVonMises>();
	material->Set_E(0.005e9);
	material->Set_v(0.3);

	std::shared_ptr<chrono::fea::ChVisualizationFEAmesh> visualizationMesh = std::make_shared<chrono::fea::ChVisualizationFEAmesh>(*(std::dynamic_pointer_cast<chrono::fea::ChMesh>(beamMesh).get()));
	visualizationMesh->SetFEMdataType(chrono::fea::ChVisualizationFEAmesh::E_PLOT_ELEM_STRAIN_VONMISES);
	visualizationMesh->SetColorscaleMinMax(-0.5, 0.5);
	visualizationMesh->SetSmoothFaces(true);
	visualizationMesh->SetWireframe(false);

	beamMesh->AddAsset(visualizationMesh);

	//Thos whole for block is for the actual building and setting of the beam
	auto nodes = CreateNodes(beamMesh, blockSize, size, physicsItemsBeam);
	for (size_t levelIndex = 0; levelIndex < nodes.size() - 1; ++levelIndex)
	{
		for (size_t lineIndex = 0; lineIndex < nodes.front().size() - 1; ++lineIndex)
		{
			for (size_t columnIndex = 0; columnIndex < nodes.front().front().size() - 1; ++columnIndex)
			{
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

	for (auto item : physicsItemsBeam)
	{
		system->Add(item);
	}

	//Render setup
	application->AssetBindAll();
	application->AssetUpdateAll();
	system->Setup();
	system->SetSolverType(chrono::ChSolver::Type::MINRES);

	//Rendering infinite while loop
	while (application->GetDevice()->run())
	{
		application->BeginScene();
		application->DrawAll();
		application->DoStep();
		application->EndScene();
	}

	return 0;
}
