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
	for (const auto& node : baseNodes)
	{
		auto constraint = std::make_shared<chrono::fea::ChLinkPointFrame>(); // Creem un link
		constraint->Initialize(node, wall); // Dam link la nod si peretele nostru
		physicsItems.emplace_back(constraint); // Adaugam obiectul generat in lista pentru eventuala randare 
	}
}

void BuildBlock(const std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>& nodes, 
	std::shared_ptr<chrono::fea::ChMesh>& mesh, 
	std::shared_ptr<chrono::fea::ChContinuumElastic>& material)
{
	auto BuildTetra = [&](const auto& firstNode, const auto& secondNode, const auto& thirdNode, const auto& fourthNode)
	{
		auto tetra = std::make_shared<chrono::fea::ChElementTetra_4>(); // Creem un obiect de tip tetraedru
		tetra->SetNodes(firstNode, secondNode, thirdNode, fourthNode); // Setam nodurile tetraedrului 
		tetra->SetMaterial(material); //Setam materialul tetraedrului
		mesh->AddElement(tetra); // Dupa ce l-am cosntruit il adaugam barnei
	}; // Creearea tetraedru unui tetraedru

	// Numarul minim de tetraedre pentru a umple un cub este 5
	// https://www.ics.uci.edu/~eppstein/projects/tetra/
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
	try
	{
		int length = size.x() + 1, height = size.y() + 1, width = size.z() + 1; // Sa fie un +1 ca obiectul sa aibe la fel de multe cuburi cate se mentioneaza in constructor (nodurile sunt cu 1 mai multa ca cuburile)
		double xSize = blockSize.x(), ySize = blockSize.y(), zSize = blockSize.z(); // Cat de departat sa fie un nod de celalat, astfel cuburile o sa fie mai mari sau mai mici
		std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>> baseNodes; // Nodurile care sunt lipite de perete

		std::vector<std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>> nodes
		(height, std::vector<std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>>
			(width, std::vector<std::shared_ptr<chrono::fea::ChNodeFEAxyz>>(length))); // Toate nodurile pe care le creem

		for (size_t levelIndex = 0; levelIndex < height; ++levelIndex) // Creem toate nodurile in spatiul 3D pentru barna
		{
			for (size_t lineIndex = 0; lineIndex < width; ++lineIndex)
			{
				for (size_t columnIndex = 0; columnIndex < length; ++columnIndex)
				{
					auto node = std::make_shared<chrono::fea::ChNodeFEAxyz>(
						chrono::Vector(lineIndex * xSize, levelIndex * ySize, columnIndex * zSize)
						); // Creem un nod la pozitia data
					nodes[levelIndex][lineIndex][columnIndex] = node; // Adaugam la lista de noduri
					mesh->AddNode(node); // Adaugam nodurile la barna
					if (lineIndex == 0)
					{
						baseNodes.emplace_back(node); // Adaugam la nodurile de langa perete
					}
				}
			}
		}
		std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyz>(mesh->GetNodes().back())->SetForce(chrono::Vector(0.0, -10000.0, 0.0)); // Setam forta care il trage in jos
		std::dynamic_pointer_cast<chrono::fea::ChNodeFEAxyz>(mesh->GetNodes().back())->SetMass(100.0); // Setam greutatea barnei
		SetFixedBase(baseNodes, physicsItems, wall); // Facem ca nodurile de langa perete sa ramana lipite de el 
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
	chrono::SetChronoDataPath(CHRONO_DATA_DIR); // Setam path-ul la chrono
	auto system = std::make_shared<chrono::ChSystemNSC>();
	auto application = std::make_shared<chrono::irrlicht::ChIrrApp>(system.get(), L"Environment", irr::core::dimension2d<irr::u32>(1200, 720), false);
	application->AddTypicalSky(); // Sa fie un cer, altfel va fi negru
	application->AddTypicalLights(); // Sa fie lumina altfel va fi totul negru -w-
	application->AddTypicalCamera(irr::core::vector3df(3, 3, 3)); // Sa adaugati voi o camera, altfel o da pe cea default

	//Create wall
	std::shared_ptr<chrono::ChBodyEasyBox> wall = std::make_shared<chrono::ChBodyEasyBox>(5, 0.1, 5, 1000, false, true);
	wall->SetBodyFixed(true); // Sa faca peretele sa nu fie afectat de fizica
	wall->SetPos(chrono::Vector(0, 0, 0)); // Pozitia peretelui in spatiul 3D
	wall->SetRot(Q_from_AngAxis(chrono::CH_C_PI / 2, chrono::VECT_Z)); //Rotatia peretelui in spatiu folosind Quternion (Pro tip: NU folositi quaternion de unii singuri, mereu vor fi alte functii care va vor ajuta sa il aflati)

	//Create beam
	std::shared_ptr<chrono::fea::ChMesh> beamMesh = std::make_shared<chrono::fea::ChMesh>();
	std::vector<std::shared_ptr<chrono::ChPhysicsItem>> physicsItemsBeam; //lista de physics item (adica toate obiectele), ca dupa sa le adaugam pe toate in sistem si sa le randam
	chrono::Vector blockSize = chrono::Vector(0.3); //un obiect cu 3 valori double initializate toate cu numarul dat
	chrono::ChVector<int> size = chrono::ChVector<int>(2, 2, 7); //un obiect cu 3 valori int initializate cu cu cele 3 numere

	physicsItemsBeam.emplace_back(beamMesh); // Adaugam obiectul general in lista pentru eventuala randare 

	std::shared_ptr<chrono::fea::ChContinuumElastic> material = std::make_shared<chrono::fea::ChContinuumPlasticVonMises>(); // materialul obiectului
	material->Set_E(0.005e9); // Elasticitate
	material->Set_v(0.3); // Cat de mult incearca sa se intoarca la forma initiala ?

	auto visualizationMesh = std::make_shared<chrono::fea::ChVisualizationFEAmesh>(
		*(std::dynamic_pointer_cast<chrono::fea::ChMesh>(beamMesh).get())
		); //Un nou mesh vizual custom pentru obiect (Altfel il va avea pe cel default)
	visualizationMesh->SetFEMdataType(chrono::fea::ChVisualizationFEAmesh::E_PLOT_ELEM_STRAIN_VONMISES); //Ce fel de stres sa il vizualizeze
	visualizationMesh->SetColorscaleMinMax(-0.5, 0.5); // Cat de mare sa fie stresul sa ajunga la minim sau maxim

	beamMesh->AddAsset(visualizationMesh);  // Ii adaugam mesha visuala creat de noi

	//Thos whole for block is for the actual building and setting of the beam
	auto nodes = CreateNodes(beamMesh, blockSize, size, physicsItemsBeam, wall); // Creem toate nodurile care vrem sa le folosim pentru barna
	for (size_t levelIndex = 0; levelIndex < nodes.size() - 1; ++levelIndex) // Parcurgem toate nodurile create pentru barna
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
					}, beamMesh, material); // Functia cu care construim un cub pentru nodurile curente
			}
		}
	}

	for (auto item : physicsItemsBeam) // Randarea tuturor obiectelor adaugate in lista
	{
		system->Add(item); // Sa adaugam fieecare obiect in system ca sa fie randat
	}
	system->Add(wall); // Sa adaugam peretele in system ca sa fie randat

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
