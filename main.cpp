// main.cpp will be used for testing the code parts under development

//#include <cstdio>
#include <iostream>
#include <il/Array.h>
#include <il/Array2D.h>
#include <il/linear_algebra/dense/factorization/LU.h>
#include <il/linear_algebra.h>
//#include <il/linear_algebra/dense/factorization/linear_solve.h>
#include "mesh_file_io.h"
#include "matrix_assembly.h"
#include "element_utilities.h"
//#include <complex>
//#include <il/StaticArray.h>
//#include <il/StaticArray2D.h>
//#include "ele_base.h"

int main() {

    double mu = 1.0, nu = 0.35;

    std::string work_directory{"C:/Users/nikolski/ClionProjects/HFPx3D_static"
                                       "/Test_Output/"};

/*
    // Testing kernel integration for one element

    il::StaticArray2D<il::int_t, 3, 1> mesh_conn;
    il::StaticArray2D<double, 3, 3> nodes_crd;
    for (il::int_t k = 0; k < 3; ++k) {
        mesh_conn(k, 0) = k;
    }
    nodes_crd(0, 0) = 0.0;
    nodes_crd(1, 0) = 0.1;
    nodes_crd(2, 0) = 0.0;
    nodes_crd(0, 1) = 1.8;
    nodes_crd(1, 1) = 0.0;
    nodes_crd(2, 1) = 0.0;
    nodes_crd(0, 2) = 1.2;
    nodes_crd(1, 2) = 1.8;
    nodes_crd(2, 2) = 0.0;
    il::StaticArray<double, 3> m_p_crd;
    m_p_crd[0] = 1.0;
    m_p_crd[1] = 1.4;
    m_p_crd[2] = -1.21;
    il::StaticArray2D<double, 3, 3> r_tensor;
    il::StaticArray2D<std::complex<double>, 6, 6> sfm =
            hfp3d::make_el_sfm_uniform(nodes_crd, il::io, r_tensor);
    il::StaticArray<std::complex<double>, 3> tau =
            hfp3d::make_el_tau_crd(nodes_crd, r_tensor);
    hfp3d::HZ hz = hfp3d::make_el_pt_hz(nodes_crd, m_p_crd, r_tensor);
    il::StaticArray2D<double, 6, 18> bem_matrix =
            hfp3d::make_local_3dbem_submatrix(1, mu, nu, hz.h, hz.z, tau, sfm);
    std::string of_name{"test_integration_1_ele.csv"};
    std::string sf_name{"test_SFM_1_ele.csv"};

    hfp3d::save_data_to_csv(sfm, work_directory, sf_name);
    hfp3d::save_data_to_csv(bem_matrix, work_directory, of_name);
*/

/*
    // Testing matrix assembly for one element

    il::Array2D<il::int_t> mesh_conn(3, 1);
    il::Array2D<double> nodes_crd(3, 3);
    for (il::int_t k = 0; k < 3; ++k) {
        mesh_conn(k, 0) = k;
    }
    nodes_crd(0, 0) = 0.0;
    nodes_crd(1, 0) = 0.1;
    nodes_crd(2, 0) = 0.0;
    nodes_crd(0, 1) = 1.8;
    nodes_crd(1, 1) = 0.0;
    nodes_crd(2, 1) = 0.0;
    nodes_crd(0, 2) = 1.2;
    nodes_crd(1, 2) = 1.8;
    nodes_crd(2, 2) = 0.0;
    std::string of_name{"test_assembly_1_ele.csv"};

     il::int_t num_elems = mesh_conn.size(1), num_dof = 18 * num_elems;

    il::Array2D<double> bem_matrix(num_dof, num_dof);
    bem_matrix = hfp3d::make_3dbem_matrix_s(mu, nu, 0.25, mesh_conn, nodes_crd);

    hfp3d::save_data_to_csv(bem_matrix, work_directory, of_name);
*/

    // Matrix assembly for a penny-shaped crack (24 elements)

    std::string src_directory{"C:/Users/nikolski/.spyder-py3/3DBEM/"};
    std::string mesh_conn_fname{"Elems_pennymesh24el_32.npy"};
    std::string nodes_crd_fname{"Nodes_pennymesh24el_32.npy"};

    std::string mf_name{"test_assembly_24_ele.csv"};
    std::string of_name{"test_solution_24_ele.csv"};

    il::Array2D<il::int_t> mesh_conn;
    il::Array2D<double> nodes_crd;
    hfp3d::load_mesh_from_numpy_32
            (src_directory, mesh_conn_fname, nodes_crd_fname, true,
             il::io, mesh_conn, nodes_crd);

    il::int_t num_elems = mesh_conn.size(1), num_dof = 18 * num_elems;

    il::Array2D<double> bem_matrix(num_dof, num_dof);
    bem_matrix = hfp3d::make_3dbem_matrix_s(mu, nu, 0.25, mesh_conn, nodes_crd);

    hfp3d::save_data_to_csv(bem_matrix, work_directory, mf_name);

    il::Array<double> rhs(num_dof);
    for (il::int_t j = 0; j < num_elems; ++j) {
        for (int k = 0; k < 6; ++k) {
            il::int_t n = j * 6 + k;
            for (int l = 0; l < 3; ++l) {
                il::int_t dof = n * 3 + l;
                rhs[dof] = (l != 1 ? 1.0 : 0.0);
            }
        }
    }
    il::Status status{};
    il::LU<il::Array2D<double>> lu_decomposition(bem_matrix, il::io, status);
    // if (!status.ok()) {
    //     // The matrix is singular to the machine precision. You should deal with
    //     // the error.
    // }
    status.abort_on_error();
    // double cnd = lu_decomposition.condition_number(il::Norm::L2, );
    // std::cout << cnd << std::endl;
    il::Array<double> dd_v;
    dd_v = lu_decomposition.solve(rhs);
    // dd_v = il::linear_solve(bem_matrix, rhs, il::io, status);

    il::Array2D<double> dd(6 * num_elems, 6);
    for (il::int_t j = 0; j < num_elems; ++j) {
        il::StaticArray2D<double, 3, 3> el_vert;
        for (il::int_t k = 0; k < 3; ++k) {
            il::int_t n = mesh_conn(k, j);
            for (il::int_t l = 0; l < 3; ++l) {
                el_vert(l, k) = nodes_crd(l, n);
            }
        }
        il::StaticArray<il::StaticArray<double, 3>, 6> el_np;
        el_np = hfp3d::el_cp_uniform(el_vert, 0.0);
        // el_np = hfp3d::el_cp_nonuniform(el_vert, v_wts, 0.0);
        for (il::int_t k = 0; k < 6; ++k) {
            il::int_t n = j * 6 + k;
            for (il::int_t l = 0; l < 3; ++l) {
                il::int_t dof = n * 3 + l;
                dd(n, l) = el_np[k][l];
                dd(n, l + 3) = dd_v[dof];
            }
        }
    }
    hfp3d::save_data_to_csv(dd, work_directory, of_name);

/*
    // il::Status status{};
    il::save(IM_1, "C:/Users/nikolski/.spyder-py3/3DBEM/matrix_24_el.npy",
             il::io, status);
    status.abort_on_error();
*/

    return 0;
}