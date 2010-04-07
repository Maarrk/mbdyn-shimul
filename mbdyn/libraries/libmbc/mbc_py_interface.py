import sys
import os
from numpy import *

try:
        import mbc_py
except ImportError:
        print "Import Error: mbc_py module"

class mbcNodal:
	def __init__(self, path, host, port, data_and_next, verbose, rigid, nodes, labels, rot, accels):
		""" initialize the module """
		mbc_py.mbc_py_nodal_initialize(path, host, port, data_and_next, verbose, rigid, nodes, labels, rot, accels);

	def get_ptr(self):
		""" set pointers """
		self.r_k_label = mbc_py.cvar.mbc_r_k_label;
		self.r_x = mbc_py.cvar.mbc_r_x;
		self.r_theta = mbc_py.cvar.mbc_r_theta;
		self.r_r = mbc_py.cvar.mbc_r_r;
		self.r_euler_123 = mbc_py.cvar.mbc_r_euler_123;
		self.r_xp = mbc_py.cvar.mbc_r_xp;
		self.r_omega = mbc_py.cvar.mbc_r_omega;
		self.r_xpp = mbc_py.cvar.mbc_r_xpp;
		self.r_omegap = mbc_py.cvar.mbc_r_omegap;
		self.r_d_label = mbc_py.cvar.mbc_r_d_label;
		self.r_f = mbc_py.cvar.mbc_r_f;
		self.r_m = mbc_py.cvar.mbc_r_m;

		self.r_k_label_size = mbc_py.cvar.mbc_r_k_label_size;
		self.r_x_size = mbc_py.cvar.mbc_r_x_size;
		self.r_theta_size = mbc_py.cvar.mbc_r_theta_size;
		self.r_r_size = mbc_py.cvar.mbc_r_r_size;
		self.r_euler_123_size = mbc_py.cvar.mbc_r_euler_123_size;
		self.r_xp_size = mbc_py.cvar.mbc_r_xp_size;
		self.r_omega_size = mbc_py.cvar.mbc_r_omega_size;
		self.r_xpp_size = mbc_py.cvar.mbc_r_xpp_size;
		self.r_omegap_size = mbc_py.cvar.mbc_r_omegap_size;
		self.r_d_label_size = mbc_py.cvar.mbc_r_d_label_size;
		self.r_f_size = mbc_py.cvar.mbc_r_f_size;
		self.r_m_size = mbc_py.cvar.mbc_r_m_size;

		self.n_k_labels = mbc_py.cvar.mbc_n_k_labels;
		self.n_x = mbc_py.cvar.mbc_n_x;
		self.n_theta = mbc_py.cvar.mbc_n_theta;
		self.n_r = mbc_py.cvar.mbc_n_r;
		self.n_euler_123 = mbc_py.cvar.mbc_n_euler_123;
		self.n_xp = mbc_py.cvar.mbc_n_xp;
		self.n_omega = mbc_py.cvar.mbc_n_omega;
		self.n_xpp = mbc_py.cvar.mbc_n_xpp;
		self.n_omegap = mbc_py.cvar.mbc_n_omegap;
		self.n_d_labels = mbc_py.cvar.mbc_n_d_labels;
		self.n_f = mbc_py.cvar.mbc_n_f;
		self.n_m = mbc_py.cvar.mbc_n_m;

		self.n_k_labels_size = mbc_py.cvar.mbc_n_k_labels_size;
		self.n_x_size = mbc_py.cvar.mbc_n_x_size;
		self.n_theta_size = mbc_py.cvar.mbc_n_theta_size;
		self.n_r_size = mbc_py.cvar.mbc_n_r_size;
		self.n_euler_123_size = mbc_py.cvar.mbc_n_euler_123_size;
		self.n_xp_size = mbc_py.cvar.mbc_n_xp_size;
		self.n_omega_size = mbc_py.cvar.mbc_n_omega_size;
		self.n_xpp_size = mbc_py.cvar.mbc_n_xpp_size;
		self.n_omegap_size = mbc_py.cvar.mbc_n_omegap_size;
		self.n_d_labels_size = mbc_py.cvar.mbc_n_d_labels_size;
		self.n_f_size = mbc_py.cvar.mbc_n_f_size;
		self.n_m_size = mbc_py.cvar.mbc_n_m_size;

	def send(self, last):
		""" send forces to peer """
		mbc_py.mbc_py_nodal_send(last);

	def recv(self):
		""" receive kinematics from peer """
		mbc_py.mbc_py_nodal_recv();

	def destroy(self):
		""" destroy handler """
		mbc_py.mbc_py_nodal_destroy();

