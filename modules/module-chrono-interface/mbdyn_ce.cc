/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2017
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 * 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

 /*
  * With the contribution of Runsen Zhang <runsen.zhang@polimi.it>
  * during Google Summer of Code 2020
  */

#include "mbdyn_ce.h"
#include "chrono/ChConfig.h"
#include "chrono_parallel/ChDataManager.h" // for simulation of parallel system, data_manager
#include "chrono_parallel/solver/ChIterativeSolverParallel.h"
#include "chrono_parallel/physics/ChSystemParallel.h"
#include "chrono/physics/ChLinkMotionImposed.h" //for 3-D dimension
#include "chrono/motion_functions/ChFunctionPosition_line.h"
#include "chrono/motion_functions/ChFunctionRotation_spline.h"
#include "chrono/geometry/ChLineSegment.h"

using namespace chrono;
using namespace chrono::collision;

extern "C" void
MBDyn_CE_CEModel_Create(ChSystemParallelNSC *pMBDyn_CE_CEModel);

extern "C" pMBDyn_CE_CEModel_t MBDyn_CE_CEModel_Init
(std::vector<double> & MBDyn_CE_CEModel_Data,
const double* pMBDyn_CE_CEFrame, const double* MBDyn_CE_CEScale,
std::vector<MBDYN_CE_CEMODELDATA> & MBDyn_CE_CEModel_Label,
const int& MBDyn_CE_CouplingType)
{
	std::cout << "Initial MBDyn_CE_CEModel pointer:\n";
	ChSystemParallelNSC *pMBDyn_CE_CEModel = new ChSystemParallelNSC;
	MBDyn_CE_CEModel_Create(pMBDyn_CE_CEModel);
	std::cout << "C::E body_vel_start: " << (pMBDyn_CE_CEModel->SearchBodyID(1))->GetPos_dt() << "\n";
	if(pMBDyn_CE_CEModel==NULL)
	{
		std::cout << "\t\tInitial MBDyn_CE_CEModel pointer fails\n";
	}
	// initial the ground coordinate in C::E;
	// r = (-CEF1,-CEF2,-CEF3);
	// R= [CEF3,  CEF4,  CEF5 ]^T           [CEF3,  CEF6,  CEF9]
	//    [CEF6,  CEF7,  CEF8 ]     ====    [CEF4,  CEF7,  CEF10]
	//    [CEF9,  CEF10, CEF11];            [CEF5,  CEF8,  CEF11]
	ChVector<> mbdynce_temp_frameMBDyn_pos(-pMBDyn_CE_CEFrame[0], -pMBDyn_CE_CEFrame[1], -pMBDyn_CE_CEFrame[2]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_X(pMBDyn_CE_CEFrame[3], pMBDyn_CE_CEFrame[4], pMBDyn_CE_CEFrame[5]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_Y(pMBDyn_CE_CEFrame[6], pMBDyn_CE_CEFrame[7], pMBDyn_CE_CEFrame[8]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_Z(pMBDyn_CE_CEFrame[9], pMBDyn_CE_CEFrame[10], pMBDyn_CE_CEFrame[11]);
	ChMatrix33<> mbdynce_temp_frameMBDyn_rot(mbdynce_temp_frameMBDyn_rot_axis_X,mbdynce_temp_frameMBDyn_rot_axis_Y,mbdynce_temp_frameMBDyn_rot_axis_Z);
	ChFrame<> mbdynce_temp_frameMBDyn(mbdynce_temp_frameMBDyn_pos, mbdynce_temp_frameMBDyn_rot);
	// initial the gravity of C::E model
	pMBDyn_CE_CEModel->Set_G_acc(mbdynce_temp_frameMBDyn.TransformDirectionLocalToParent(ChVector<>(0.0,-9.81*MBDyn_CE_CEScale[0],0.0)));

	// initial motor for coupling bodies.
	if (MBDyn_CE_CouplingType>=-1)//coupling
	{
		unsigned mbdynce_temp_bodies_num = MBDyn_CE_CEModel_Label.size() - 1;
		unsigned mbdynce_temp_ground_id = MBDyn_CE_CEModel_Label[mbdynce_temp_bodies_num].MBDyn_CE_CEBody_Label; // ground ID is set in the last element
		MBDyn_CE_CEModel_Label[mbdynce_temp_bodies_num].MBDyn_CE_CEMotor_Label = 0; // the last label is invalid
		auto mbdynce_temp_ground = pMBDyn_CE_CEModel->SearchBodyID(mbdynce_temp_ground_id);
		for (unsigned i = 0; i < mbdynce_temp_bodies_num;i++)
		{
			unsigned body_i_id = MBDyn_CE_CEModel_Label[i].MBDyn_CE_CEBody_Label;
			auto body_i = pMBDyn_CE_CEModel->SearchBodyID(body_i_id);
			std::cout << "C::E body_vel_start after creating motor: " << (pMBDyn_CE_CEModel->SearchBodyID(1))->GetPos_dt() << "\n";
			auto motor3d_body_i = std::make_shared<ChLinkMotionImposed>();
			motor3d_body_i->Initialize(body_i,
									   mbdynce_temp_ground,
									   true,																 //connecting frames are described in local ref.
									   ChFrame<>(ChVector<>(0.0, 0.0, 0.0), ChQuaternion<>(1., 0., 0., 0.)), // By default: using the mass center and the body orientation
									   ChFrame<>(ChVector<>(0.0, 0.0, 0.0), ChQuaternion<>(1., 0., 0., 0.)));
			pMBDyn_CE_CEModel->Add(motor3d_body_i);
			auto motor3d_function_pos = std::make_shared<ChFunctionPosition_line>(); // impose veloctiy:: TO DO !!!!!!!
			auto motor3d_function_rot = std::make_shared<ChFunctionRotation_spline>();
			motor3d_body_i->SetPositionFunction(motor3d_function_pos);
			motor3d_body_i->SetRotationFunction(motor3d_function_rot);
			MBDyn_CE_CEModel_Label[i].MBDyn_CE_CEMotor_Label = motor3d_body_i->GetIdentifier();
			std::cout << "C::E motor " << i + 1 << " ID:\t" << MBDyn_CE_CEModel_Label[i].MBDyn_CE_CEMotor_Label << "\n";		
		}
		std::cout << "C::E ground ID:\t"<<mbdynce_temp_ground->GetIdentifier()<<"\n";
	}
	else
	{
		std::cout << "Coupling none in C::E model.\n";
	}
	// printf model information after adding motor
	std::cout << "there is the coupling C::E model \n";
	std::cout << "num of links:\t" << pMBDyn_CE_CEModel->Get_linklist().size() << "\n";
	std::cout << "num of other physicslist:\t" << pMBDyn_CE_CEModel->Get_otherphysicslist().size() << "\n";
	std::cout << "num of rigid bodies:\t" << pMBDyn_CE_CEModel->Get_bodylist().size() << "\n";
	std::cout << "num of speed motor:\t" << pMBDyn_CE_CEModel->data_manager->num_linmotors << "\n";
	// allocate space for C::E_Model_Data;
	unsigned int bodies_size = pMBDyn_CE_CEModel->Get_bodylist().size();
	unsigned int system_size = (3 * 3 + 3 * 4) * bodies_size + 1; // +1: save Chtime; system_size=body_size+1(time)
	MBDyn_CE_CEModel_Data.resize(system_size,0.0);
	return pMBDyn_CE_CEModel;
}

extern "C" void
MBDyn_CE_CEModel_Destroy(pMBDyn_CE_CEModel_t pMBDyn_CE_CEModel)
{
	if (pMBDyn_CE_CEModel==NULL)
	{
		std::cout << "destroy the CE_model fails:\n";
		return;
	}
	std::cout << "destroy the CE_model...\n";
	if(pMBDyn_CE_CEModel!=NULL)
	{
		// must convert to the correct type
		// delete  (int *) pMBDyn_CE_CEModel;
		delete  (ChSystemParallelNSC *) pMBDyn_CE_CEModel;
		pMBDyn_CE_CEModel = NULL;
	}
}

// save CEModel at current step for reloading them in the tight coupling scheme
// (before advance())
extern "C" int
MBDyn_CE_CEModel_DataSave(pMBDyn_CE_CEModel_t pMBDyn_CE_CEModel, 
                        std::vector<double> & MBDyn_CE_CEModel_Data)
{
	if (pMBDyn_CE_CEModel==NULL)
	{
		std::cout << "\t\tCE_models DataSave() fails:\n";
		return 0;
	}
	ChSystemParallelNSC *tempsys = (ChSystemParallelNSC *)pMBDyn_CE_CEModel;
	std::cout << "\t\tCE_models DataSave():\n";
	unsigned int tempsys_bodies_size = tempsys->Get_bodylist().size();
	unsigned int tempsys_size = (3 * 3 + 3 * 4) * tempsys_bodies_size + 1; // +1 Chtime: Sys_size=body_size + 1(for time);
	unsigned int vector_size = MBDyn_CE_CEModel_Data.size();
	if (tempsys_size != vector_size)
	{
		std::cout << "Error: the vector to save data is not consistent with the C::E model:\n"; // how to safely exit MBDyn?
		return 1;
	}

	// save data
//#pragma omp parallel for
	for (unsigned int i = 0; i < tempsys_bodies_size; i++)
	{
		const ChVector<>& body_pos = tempsys->Get_bodylist()[i]->GetPos(); // 3
		const ChQuaternion<> &body_rot = tempsys->Get_bodylist()[i]->GetRot();   // 4
		const ChVector<> &body_pos_dt = tempsys->Get_bodylist()[i]->GetPos_dt(); // 3
		const ChQuaternion<> &body_rot_dt = tempsys->Get_bodylist()[i]->GetRot_dt(); // 4
		const ChVector<> &body_pos_dtdt = tempsys->Get_bodylist()[i]->GetPos_dtdt(); // 3
		const ChQuaternion<> &body_rot_dtdt = tempsys->Get_bodylist()[i]->GetRot_dtdt(); //4

		unsigned int i_pos = (3 * 3 + 3 * 4) * i; 
		MBDyn_CE_CEModel_Data[i_pos] = body_pos.x();
		MBDyn_CE_CEModel_Data[i_pos+1] = body_pos.y();
		MBDyn_CE_CEModel_Data[i_pos+2] = body_pos.z();
		std::cout << "\t\tsave position" << i << ": " << MBDyn_CE_CEModel_Data[i_pos] << "\n";

		unsigned int i_rot = i_pos + 3;
		MBDyn_CE_CEModel_Data[i_rot] = body_rot.e0();
		MBDyn_CE_CEModel_Data[i_rot+1] = body_rot.e1();
		MBDyn_CE_CEModel_Data[i_rot+2] = body_rot.e2();
		MBDyn_CE_CEModel_Data[i_rot+3] = body_rot.e3();

		unsigned int i_pos_dt = i_rot + 4;
		MBDyn_CE_CEModel_Data[i_pos_dt] = body_pos_dt.x();
		MBDyn_CE_CEModel_Data[i_pos_dt+1] = body_pos_dt.y();
		MBDyn_CE_CEModel_Data[i_pos_dt+2] = body_pos_dt.z();

		unsigned int i_rot_dt = i_pos_dt + 3;
		MBDyn_CE_CEModel_Data[i_rot_dt] = body_rot_dt.e0();
		MBDyn_CE_CEModel_Data[i_rot_dt+1] = body_rot_dt.e1();
		MBDyn_CE_CEModel_Data[i_rot_dt+2] = body_rot_dt.e2();
		MBDyn_CE_CEModel_Data[i_rot_dt+3] = body_rot_dt.e3();

		unsigned int i_pos_dtdt = i_rot_dt + 4;
		MBDyn_CE_CEModel_Data[i_pos_dtdt] = body_pos_dtdt.x();
		MBDyn_CE_CEModel_Data[i_pos_dtdt+1] = body_pos_dtdt.y();
		MBDyn_CE_CEModel_Data[i_pos_dtdt+2] = body_pos_dtdt.z();

		unsigned int i_rot_dtdt = i_pos_dtdt + 3;
		MBDyn_CE_CEModel_Data[i_rot_dtdt] = body_rot_dtdt.e0();
		MBDyn_CE_CEModel_Data[i_rot_dtdt+1] = body_rot_dtdt.e1();
		MBDyn_CE_CEModel_Data[i_rot_dtdt+2] = body_rot_dtdt.e2();
		MBDyn_CE_CEModel_Data[i_rot_dtdt+3] = body_rot_dtdt.e3();
	}
	MBDyn_CE_CEModel_Data[tempsys_size-1] = tempsys->GetChTime();
	std::cout << "\t\tsave time: " << MBDyn_CE_CEModel_Data[tempsys_size - 1] << "\n";
	return 0;
}


// reload data in the tight coupling scheme at each iteration
extern "C" int
MBDyn_CE_CEModel_DataReload(pMBDyn_CE_CEModel_t pMBDyn_CE_CEModel, 
                        std::vector<double> & MBDyn_CE_CEModel_Data)
{
	if (pMBDyn_CE_CEModel==NULL)
	{
		std::cout << "\t\tCE_models DataReload() fails:\n";
		return 0;
	}
	std::cout << "\t\tCE_models DataReload():\n";
	ChSystemParallelNSC *tempsys = (ChSystemParallelNSC *)pMBDyn_CE_CEModel;
	unsigned int tempsys_bodies_size = tempsys->Get_bodylist().size();
	unsigned int tempsys_size = (3 * 3 + 3 * 4) * tempsys_bodies_size + 1; // +1 Chtime: Sys_size=body_size + 1(for time);
	unsigned int vector_size = MBDyn_CE_CEModel_Data.size();
	if (tempsys_size != vector_size)
	{
		std::cout << "Error: the vector to save data is not consistent with the C::E model:\n"; // how to safely exit MBDyn?
		return 1;
	}
	double MBDyn_CE_CEModel_time = MBDyn_CE_CEModel_Data[tempsys_size - 1];
	tempsys->SetChTime(MBDyn_CE_CEModel_time);
//#pragma omp parallel for
	for (int i = 0; i < tempsys_bodies_size; i++)
	{
		unsigned int i_pos = (3 * 3 + 3 * 4) * i;
		tempsys->Get_bodylist()[i]->SetPos(ChVector<>(MBDyn_CE_CEModel_Data[i_pos],
										 MBDyn_CE_CEModel_Data[i_pos + 1],
										 MBDyn_CE_CEModel_Data[i_pos + 2]));
		std::cout << "\t\treload position" << i << ": " << MBDyn_CE_CEModel_Data[i_pos] << "\n";
		unsigned int i_rot = i_pos + 3;
		tempsys->Get_bodylist()[i]->SetRot(ChQuaternion<>(MBDyn_CE_CEModel_Data[i_rot],
										 MBDyn_CE_CEModel_Data[i_rot + 1],
										 MBDyn_CE_CEModel_Data[i_rot + 2],
										 MBDyn_CE_CEModel_Data[i_rot + 3]));

		unsigned int i_pos_dt = i_rot + 4;
		tempsys->Get_bodylist()[i]->SetPos_dt(ChVector<>(MBDyn_CE_CEModel_Data[i_pos_dt],
													  MBDyn_CE_CEModel_Data[i_pos_dt + 1],
													  MBDyn_CE_CEModel_Data[i_pos_dt + 2]));

		unsigned int i_rot_dt = i_pos_dt + 3;
		tempsys->Get_bodylist()[i]->SetRot_dt(ChQuaternion<>(MBDyn_CE_CEModel_Data[i_rot_dt],
														  MBDyn_CE_CEModel_Data[i_rot_dt + 1],
														  MBDyn_CE_CEModel_Data[i_rot_dt + 2],
														  MBDyn_CE_CEModel_Data[i_rot_dt + 3]));
		
		unsigned int i_pos_dtdt = i_rot_dt + 4;
		tempsys->Get_bodylist()[i]->SetPos_dtdt(ChVector<>(MBDyn_CE_CEModel_Data[i_pos_dtdt],
													  MBDyn_CE_CEModel_Data[i_pos_dtdt + 1],
													  MBDyn_CE_CEModel_Data[i_pos_dtdt + 2]));

		unsigned int i_rot_dtdt = i_pos_dtdt + 3;
		tempsys->Get_bodylist()[i]->SetRot_dtdt(ChQuaternion<>(MBDyn_CE_CEModel_Data[i_rot_dtdt],
														  MBDyn_CE_CEModel_Data[i_rot_dtdt + 1],
														  MBDyn_CE_CEModel_Data[i_rot_dtdt + 2],
														  MBDyn_CE_CEModel_Data[i_rot_dtdt + 3]));
		tempsys->Get_bodylist()[i]->Update(MBDyn_CE_CEModel_time);
	}
	return 0;
}

extern "C" void
MBDyn_CE_CEModel_DoStepDynamics(pMBDyn_CE_CEModel_t pMBDyn_CE_CEModel, double time_step)
{
	if (pMBDyn_CE_CEModel==NULL)
	{
		std::cout << "\t\tCE_models DoStepDynamics() fails:\n";
		return;
	}
	std::cout << "\t\tCE_models DoStepDynamics():\n";
	if (pMBDyn_CE_CEModel!=NULL)
	{   // it's not a good idea to do this convert
		// it's dangerous, how to avoid?
		ChSystemParallelNSC* tempsys=(ChSystemParallelNSC *) pMBDyn_CE_CEModel;
		tempsys->DoStepDynamics(time_step);
		tempsys->CalculateContactForces();
		std::cout << "\t\ttime: " << tempsys->GetChTime() << " s\n";
		std::cout << "\t\ttime step: " << time_step << " s\n";
		std::cout << "\t\tpos after integration: " << tempsys->SearchBodyID(1)->GetPos() << "\n";
	}
}

// C::E models receive coupling motion from the buffer
void 
MBDyn_CE_CEModel_RecvFromBuf(pMBDyn_CE_CEModel_t pMBDyn_CE_CEModel, 
const std::vector<double>& MBDyn_CE_CouplingKinematic, 
const unsigned& MBDyn_CE_NodesNum,
const std::vector<MBDYN_CE_CEMODELDATA> & MBDyn_CE_CEModel_Label,
double time_step)
{
	if (pMBDyn_CE_CEModel==NULL)
	{
		std::cout << "\t\tCE_models RecvFromMBDyn() fails:\n";
		return;
	}
	std::cout << "\t\tCE_models RecvFromMBDyn():\n";
	ChSystemParallelNSC *mbdynce_tempsys = (ChSystemParallelNSC *)pMBDyn_CE_CEModel;
	// 1. obtain the data;
	// 2. transfer it to the coordinate in C::E;
	// 3. update motor functions;

	// 1. obtain the data;
	const double *pmbdynce_tempvec3_x = &MBDyn_CE_CouplingKinematic[0];
	const double *pmbdynce_tempemat3x3_R = &MBDyn_CE_CouplingKinematic[3 * MBDyn_CE_NodesNum];
	const double *pmbdynce_tempvec3_xp = &MBDyn_CE_CouplingKinematic[12 * MBDyn_CE_NodesNum];
	const double *pmbdynce_tempvec3_omega = &MBDyn_CE_CouplingKinematic[15 * MBDyn_CE_NodesNum];
	const double *pmbdynce_tempvec3_xpp = &MBDyn_CE_CouplingKinematic[18 * MBDyn_CE_NodesNum];
	const double *pmbdynce_tempvec3_omegap = &MBDyn_CE_CouplingKinematic[21 * MBDyn_CE_NodesNum];
	const double *pmbdynce_temp_frame = &MBDyn_CE_CouplingKinematic[24 * MBDyn_CE_NodesNum];
	ChVector<> mbdynce_temp_frameMBDyn_pos(-pmbdynce_temp_frame[0], -pmbdynce_temp_frame[1], -pmbdynce_temp_frame[2]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_X(pmbdynce_temp_frame[3], pmbdynce_temp_frame[4], pmbdynce_temp_frame[5]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_Y(pmbdynce_temp_frame[6], pmbdynce_temp_frame[7], pmbdynce_temp_frame[8]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_Z(pmbdynce_temp_frame[9], pmbdynce_temp_frame[10], pmbdynce_temp_frame[11]);
	ChMatrix33<> mbdynce_temp_frameMBDyn_rot(mbdynce_temp_frameMBDyn_rot_axis_X,mbdynce_temp_frameMBDyn_rot_axis_Y,mbdynce_temp_frameMBDyn_rot_axis_Z);
	ChFrame<> mbdynce_temp_frameMBDyn(mbdynce_temp_frameMBDyn_pos, mbdynce_temp_frameMBDyn_rot);
	double time = mbdynce_tempsys->GetChTime();
	// 2. transfer it to the coordinate in C::E, and 3. update motor functions
	for (unsigned i = 0; i < MBDyn_CE_NodesNum;i++)
	{
		// 2.1 coordinate transformation
		// Currently, the codes only use the pos and rotation.
		ChVector<> mbdynce_tempmbdyn_pos = ChVector<>(pmbdynce_tempvec3_x[3 * i], pmbdynce_tempvec3_x[3 * i + 1], pmbdynce_tempvec3_x[3 * i + 2]) >> mbdynce_temp_frameMBDyn;
		ChMatrix33<> mbdynce_tempmbdyn_R1(ChVector<>(pmbdynce_tempemat3x3_R[9 * i], pmbdynce_tempemat3x3_R[9 * i + 3], pmbdynce_tempemat3x3_R[9 * i + 6]),
										  ChVector<>(pmbdynce_tempemat3x3_R[9 * i + 1], pmbdynce_tempemat3x3_R[9 * i + 4], pmbdynce_tempemat3x3_R[9 * i + 7]),
										  ChVector<>(pmbdynce_tempemat3x3_R[9 * i + 2], pmbdynce_tempemat3x3_R[9 * i + 5], pmbdynce_tempemat3x3_R[9 * i + 8])); // three column vectors
		ChMatrix33<> mbdynce_tempmbdyn_R(mbdynce_tempmbdyn_R1.Get_A_quaternion() >> mbdynce_temp_frameMBDyn);
		ChFrame<> mbdynce_tempframeG_end(mbdynce_tempmbdyn_pos,mbdynce_tempmbdyn_R);
		// 2.2 create motor functions
		// find motor i
		unsigned motor3d_body_i_id = MBDyn_CE_CEModel_Label[i].MBDyn_CE_CEMotor_Label;
		std::cout << "\t\tmotor3d_body_i_id is: " << motor3d_body_i_id << std::endl;
		auto motor3d_body_i = std::dynamic_pointer_cast<ChLinkMotionImposed>(mbdynce_tempsys->SearchLinkID(motor3d_body_i_id));
		// frame_start // more details:: TO DO !!!
		if (motor3d_body_i!=NULL)
		{
			auto motor3d_function_pos = std::dynamic_pointer_cast<ChFunctionPosition_line>(motor3d_body_i->GetPositionFunction());
			auto motor3d_function_rot = std::dynamic_pointer_cast<ChFunctionRotation_spline>(motor3d_body_i->GetRotationFunction());
			if (motor3d_function_pos != NULL & motor3d_function_rot != NULL)
			{
				ChFrame<> mbdynce_tempframe1b1_start, mbdynce_tempframe1G_start, mbdynce_tempframeM2_start, mbdynce_tempframeM2_end;
				if (time < time_step)
				{
					mbdynce_tempframeM2_start=ChFrame<>(ChVector<>(0.0,0.0,0.0),motor3d_body_i->GetBody1()->GetRot());
				}
				else
				{
					mbdynce_tempframe1b1_start = motor3d_body_i->GetFrame1();
					mbdynce_tempframe1G_start = mbdynce_tempframe1b1_start >> *(motor3d_body_i->GetBody1());
					mbdynce_tempframeM2_start=ChFrame<>(mbdynce_tempframe1G_start >> ((motor3d_body_i->GetFrame2()) >> *(motor3d_body_i->GetBody2())).GetInverse()); // expressed in Frame 2
				}
				mbdynce_tempframeM2_end = ChFrame<>(mbdynce_tempframeG_end >> ((motor3d_body_i->GetFrame2()) >> *motor3d_body_i->GetBody2()).GetInverse()); // expressed in Frame 2
				std::cout << "\t\tC::E model motions_pos_start: " << mbdynce_tempframeM2_start.GetPos() << "\n";
				std::cout << "\t\tC::E model motions_rot_start: " << mbdynce_tempframeM2_start.GetRot()<< "\n";
				std::cout << "\t\tC::E model motions_pos_end: " << mbdynce_tempframeM2_end.GetPos() << "\n";
				std::cout << "\t\tC::E model motions_rot_end: " << mbdynce_tempframeM2_end.GetRot() << "\n";
				// position function
				auto mbdynce_temp_pos_line = chrono_types::make_shared<geometry::ChLineSegment>(mbdynce_tempframeM2_start.GetPos(), mbdynce_tempframeM2_end.GetPos());
				motor3d_function_pos->SetLine(mbdynce_temp_pos_line);
				// chrono_types::make_shared<>: a more safety case in C::E
				motor3d_function_pos->SetSpaceFunction(chrono_types::make_shared<ChFunction_Ramp>(-time / time_step, 1 / time_step));
				// rotation function
				std::vector<ChQuaternion<>> mbdynce_temp_rot_spline = {{mbdynce_tempframeM2_start.GetRot()}, {mbdynce_tempframeM2_end.GetRot()}};
				motor3d_function_rot->SetupData(1, mbdynce_temp_rot_spline);
				motor3d_function_rot->SetSpaceFunction(chrono_types::make_shared<ChFunction_Ramp>(-time / time_step, 1 / time_step));
			}
		}
	}
}

// C::E models send coupling forces to the buffer
void 
MBDyn_CE_CEModel_SendToBuf(pMBDyn_CE_CEModel_t pMBDyn_CE_CEModel, std::vector<double> &MBDyn_CE_CouplingDynamic, 
							double* pMBDyn_CE_CEFrame, const unsigned& MBDyn_CE_NodesNum, const double* MBDyn_CE_CEScale,
							const std::vector<MBDYN_CE_CEMODELDATA> & MBDyn_CE_CEModel_Label)
{
	if (pMBDyn_CE_CEModel==NULL)
	{
		std::cout << "\t\tCE_models SendToMBDyn() fails:\n";
		return;
	}
	std::cout << "\t\tCE_models SendToMBDyn():\n";
	ChSystemParallelNSC *mbdynce_tempsys = (ChSystemParallelNSC *)pMBDyn_CE_CEModel;
	// obtain the transform matrix
	ChVector<> mbdynce_temp_frameMBDyn_pos(-pMBDyn_CE_CEFrame[0], -pMBDyn_CE_CEFrame[1], -pMBDyn_CE_CEFrame[2]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_X(pMBDyn_CE_CEFrame[3], pMBDyn_CE_CEFrame[4], pMBDyn_CE_CEFrame[5]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_Y(pMBDyn_CE_CEFrame[6], pMBDyn_CE_CEFrame[7], pMBDyn_CE_CEFrame[8]);
	ChVector<> mbdynce_temp_frameMBDyn_rot_axis_Z(pMBDyn_CE_CEFrame[9], pMBDyn_CE_CEFrame[10], pMBDyn_CE_CEFrame[11]);
	ChMatrix33<> mbdynce_temp_frameMBDyn_rot(mbdynce_temp_frameMBDyn_rot_axis_X,mbdynce_temp_frameMBDyn_rot_axis_Y,mbdynce_temp_frameMBDyn_rot_axis_Z);
	ChFrame<> mbdynce_temp_frameMBDyn(mbdynce_temp_frameMBDyn_pos, mbdynce_temp_frameMBDyn_rot);
	// write exchange force/torque to the buffer
	ChVector<> mbdynce_ce_force_G(0.0, 0.0, 0.0);
	ChVector<> mbdynce_ce_torque_G(0.0, 0.0, 0.0);
	ChVector<> mbdynce_mbdyn_force = mbdynce_temp_frameMBDyn.TransformDirectionParentToLocal(mbdynce_ce_force_G);
	ChVector<> mbdynce_mbdyn_torque = mbdynce_temp_frameMBDyn.TransformDirectionParentToLocal(mbdynce_ce_force_G);
	double* pmbdynce_tempvec3_f = &MBDyn_CE_CouplingDynamic[0];
	double* pmbdynce_tempvec3_m = &MBDyn_CE_CouplingDynamic[3*MBDyn_CE_NodesNum];
	for (unsigned i = 0; i < MBDyn_CE_NodesNum; i++)
	{
		// obtain the exchange force/torque from CE model; 
		unsigned motor_i_id = MBDyn_CE_CEModel_Label[i].MBDyn_CE_CEMotor_Label;
		auto motor_i = std::dynamic_pointer_cast<ChLinkMotionImposed>(mbdynce_tempsys->SearchLinkID(motor_i_id));
		if (motor_i==NULL)
		{
			std::cout << "\t\tcannot read coupling motor\n";
		}
		else
		{
			
			ChCoordsys<> mbdynce_temp_frameMG_calcu (motor_i->GetLinkAbsoluteCoords());
			//!!!!(Here I think that the get_react_force() obtains reaction forces applied to body 2 in the link frame, not the forces
			//applied to body 1 in the link frame)
			//get the reaction force at the global frame and MBDyn frame, apply to the node
			mbdynce_ce_force_G=mbdynce_temp_frameMG_calcu.TransformDirectionLocalToParent(motor_i->Get_react_force());
			mbdynce_ce_torque_G=mbdynce_temp_frameMG_calcu.TransformDirectionLocalToParent(motor_i->Get_react_torque());
			mbdynce_mbdyn_force = mbdynce_temp_frameMBDyn.TransformDirectionParentToLocal(mbdynce_ce_force_G);
			mbdynce_mbdyn_torque = mbdynce_temp_frameMBDyn.TransformDirectionParentToLocal(mbdynce_ce_torque_G);
			std::cout << "\t\tcoupling forces_f: " << mbdynce_mbdyn_force << "\n";
			std::cout << "\t\tcoupling forces_q: " << mbdynce_mbdyn_torque << "\n";
			// get the contact force
			// ChVector<> contact_forc(system.GetBodyContactForce(container).x, system.GetBodyContactForce(container).y, system.GetBodyContactForce(container).z);
			// ChVector<> contact_torq(system.GetBodyContactTorque(container).x, system.GetBodyContactTorque(container).y, system.GetBodyContactTorque(container).z);
			// mbdynce_ce_force_G=mbdynce_temp_frameMG_calcu.TransformDirectionLocalToParent(contact_forc);
			// mbdynce_ce_torque_G=mbdynce_temp_frameMG_calcu.TransformDirectionLocalToParent(contact_torq);
			// mbdynce_mbdyn_force = mbdynce_temp_frameMBDyn.TransformDirectionParentToLocal(mbdynce_ce_force_G);
			// mbdynce_mbdyn_force = mbdynce_temp_frameMBDyn.TransformDirectionParentToLocal(mbdynce_ce_torque_G);
		}
		// write in the buffer
		double mbdynce_tempvec3_f[3]={mbdynce_mbdyn_force.x()/MBDyn_CE_CEScale[2],//0.0,0.0};
										mbdynce_mbdyn_force.y()/MBDyn_CE_CEScale[2],
										mbdynce_mbdyn_force.z()/MBDyn_CE_CEScale[2]};
		std::cout << "\t\tcoupling forces_f: " << mbdynce_tempvec3_f[0] << "\n";
		double mbdynce_tempvec3_m[3]={mbdynce_mbdyn_torque.x()/(MBDyn_CE_CEScale[3]),
										mbdynce_mbdyn_torque.y()/(MBDyn_CE_CEScale[3]),
										mbdynce_mbdyn_torque.z()/(MBDyn_CE_CEScale[3])};
		memcpy(&pmbdynce_tempvec3_f[3*i], &mbdynce_tempvec3_f[0], 3 * sizeof(double));
		memcpy(&pmbdynce_tempvec3_m[3*i], &mbdynce_tempvec3_m[0], 3 * sizeof(double));
	}
}