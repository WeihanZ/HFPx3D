//
// This file is part of 3d_bem.
//
// Created by D. Nikolski on 1/24/2017.
// Copyright (c) ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE, Switzerland,
// Geo-Energy Laboratory, 2016-2017.  All rights reserved.
// See the LICENSE.TXT file for more details. 
//

#include <cmath>
#include <complex>
#include <il/StaticArray.h>
#include <il/StaticArray2D.h>
#include <il/linear_algebra/dense/blas/dot.h>
#include <il/linear_algebra/dense/norm.h>
#include "ele_base.h"

namespace hfp3d {

// auxiliary functions (norm, cross product)

    double l2norm(const il::StaticArray<double, 3> &a) {
// L2 norm of a vector
        double n_a = 0.0;
        for (int k = 0; k < a.size(); ++k) {
            n_a += a[k] * a[k];
        }
        n_a = std::sqrt(n_a);
        return n_a;
    }

    il::StaticArray<double, 3> normalize
            (const il::StaticArray<double, 3> &a) {
// normalized 3D vector
        il::StaticArray<double, 3> e;
        //double n_a = l2norm(a);
        double n_a = il::norm(a, il::Norm::L2);
        for (int k = 0; k < a.size(); ++k) {
            e[k] = a[k] / n_a;
        }
        return e;
    }

    il::StaticArray<double, 3> cross(const il::StaticArray<double, 3> &a,
                                     const il::StaticArray<double, 3> &b) {
// cross product of two 3D vectors
        IL_EXPECT_FAST(a.size() == 3);
        IL_EXPECT_FAST(b.size() == 3);
        il::StaticArray<double, 3> c;
        c[0] = a[1] * b[2] - a[2] * b[1];
        c[1] = a[2] * b[0] - a[0] * b[2];
        c[2] = a[0] * b[1] - a[1] * b[0];
        return c;
    }

// Element's local coordinate system manipulations

    il::StaticArray2D<double, 3, 3> make_el_r_tensor
            (const il::StaticArray2D<double, 3, 3> &el_vert) {
// This function calculates the rotation tensor -
// coordinate transform from "global" (reference) 
// Cartesian coordinate system to the element's local 
// Cartesian coordinate system with origin at the 
// first vertex of the element (el_vert(j, 0))
        il::StaticArray2D<double, 3, 3> r_tensor;
        il::StaticArray<double, 3> a1{}, a2{}, a3{},
                e1{}, e2{}, e3{};
        for (int j = 0; j < 3; ++j) {
            a1[j] = el_vert(j, 1) - el_vert(j, 0);
            a2[j] = el_vert(j, 2) - el_vert(j, 0);
        }
        e1 = normalize(a1);
        a3 = cross(e1, a2);
        e3 = normalize(a3);
        e2 = normalize(cross(e3, e1));
        for (int j = 0; j < 3; ++j) {
            r_tensor(0, j) = e1[j];
            r_tensor(1, j) = e2[j];
            r_tensor(2, j) = e3[j];
        }
        return r_tensor;
    }

    il::StaticArray<std::complex<double>, 3> make_el_tau_crd
            (const il::StaticArray2D<double, 3, 3> &el_vert,
             const il::StaticArray2D<double, 3, 3> &r_tensor) {
// This function calculates the tau-coordinates
// of the element's vertices
// r-tensor is the rotation tensor for the
// coordinate transform from "global" (reference) 
// Cartesian coordinate system to the element's local one
// Note: use il::dot(r_tensor, a) for R.a
// and il::dot(r_tensor, il::Blas::transpose, a) for R^T.a
        il::StaticArray<std::complex<double>, 3> tau{0.0};
        il::StaticArray<double, 3> loc_origin;
        il::StaticArray2D<double, 3, 3> b_vects{0.0};
        for (int k = 0; k < 3; ++k) {
            // Here the 1st vertex is chosen as the origin
            loc_origin[k] = el_vert(k, 0);
        }
        for (int k = 0; k < 3; ++k) {
            for (int j = 0; j < 3; ++j) {
                // Basis vectors (rows)
                b_vects(k, j) = el_vert(k, j) - loc_origin[k];
            }
        }
        // Rotated b_vects
        il::StaticArray2D<double, 3, 3> b_v_r = il::dot(r_tensor, b_vects);
        for (int k = 0; k < 3; ++k) {
            tau[k] = std::complex<double>(b_v_r(0, k), b_v_r(1, k));
        }
        return tau;
    }

    il::StaticArray2D<std::complex<double>, 2, 2> make_el_tau_2_mc
            (const il::StaticArray2D<double, 3, 3> &el_vert,
             const il::StaticArray2D<double, 3, 3> &r_tensor) {
// This function calculates the coordinate transform
// from local Cartesian coordinates to "master element"
// ([tau, conj(tau)] to [x,y]) with origin at the 
// first vertex of the element (el_vert(j, 0))
        il::StaticArray2D<std::complex<double>, 2, 2> transform_mtr{0.0};
        il::StaticArray<std::complex<double>, 2> z23{0.0};
        il::StaticArray<double, 3> xsi{0.0}, vv{0.0};
        std::complex<double> m_det;
        for (int k = 0; k < 2; ++k) {
            for (int n = 0; n < 3; ++n) {
                vv[n] = el_vert(n, k + 1) - el_vert(n, 0);
            }
            xsi = il::dot(r_tensor, vv);
            z23[k] = std::complex<double>(xsi[0], xsi[1]);
        }
        // common denominator (determinant)
        m_det = z23[0] * std::conj(z23[1]) - z23[1] * std::conj(z23[0]);
        // inverse transform
        transform_mtr(0, 0) = std::conj(z23[1]) / m_det;
        transform_mtr(0, 1) = -z23[1] / m_det;
        transform_mtr(1, 0) = -std::conj(z23[0]) / m_det;
        transform_mtr(1, 1) = z23[0] / m_det;
        return transform_mtr;
    }

    HZ make_el_pt_hz
            (const il::StaticArray2D<double, 3, 3> &el_vert,
             const il::StaticArray<double, 3> &m_pt_crd,
             const il::StaticArray2D<double, 3, 3> &r_tensor) {
// This function calculates the h- and tau-coordinates 
// of the point m_pt_cr with respect to the element's 
// local Cartesian coordinate system with origin at 
// the first vertex of the element (el_vert(j, 0))
        HZ h_z;
        il::StaticArray<double, 3> loc_origin, m_pt_c_r;
        for (int k = 0; k < 3; ++k) {
            loc_origin[k] = el_vert(k, 0);
            m_pt_c_r[k] = m_pt_crd[k] - loc_origin[k];
        }
        m_pt_c_r = il::dot(r_tensor, m_pt_c_r);
        h_z.h = -m_pt_c_r[2];
        h_z.z = std::complex<double>(m_pt_c_r[0], m_pt_c_r[1]);
        return h_z;
    }

// Element's basis (shape) functions

    il::StaticArray2D<std::complex<double>, 6, 6> make_el_sfm_uniform
            (const il::StaticArray2D<double, 3, 3> &el_vert,
             il::io_t, il::StaticArray2D<double, 3, 3> &r_tensor) {
// This function calculates the basis (shape) functions' 
// coefficients (rows of sfm) for a triangular boundary 
// element with 2nd order polynomial approximation of unknowns
// in terms of complex (tau, conj(tau)) representation of 
// local element's coordinates with trivial (middle) 
// edge partitioning;
// returns the same as 
// make_el_sfm_nonuniform(el_vert, {1.0, 1.0, 1.0})

        // tau_2_mc defines inverse coordinate transform 
        // [tau, conj(tau)] to [x,y] (see make_el_tau_2_mc)
        r_tensor = make_el_r_tensor(el_vert);
        il::StaticArray2D<std::complex<double>, 2, 2> tau_2_mc = 
                make_el_tau_2_mc(el_vert, r_tensor);
        il::StaticArray2D<std::complex<double>, 6, 6> sfm{0.0}, sfm_mc{0.0};

        // coefficients of shape functions (rows) 
        // for master element (0<=x,y<=1); ~[1, x, y, x^2, y^2, x*y]
        sfm_mc(0, 0) = 1.0;
        sfm_mc(0, 1) = -3.0;
        sfm_mc(0, 2) = -3.0;
        sfm_mc(0, 3) = 2.0;
        sfm_mc(0, 4) = 2.0;
        sfm_mc(0, 5) = 4.0;
        sfm_mc(1, 1) = -1.0;
        sfm_mc(1, 3) = 2.0;
        sfm_mc(2, 2) = -1.0;
        sfm_mc(2, 4) = 2.0;
        sfm_mc(3, 5) = 4.0;
        sfm_mc(4, 2) = 4.0;
        sfm_mc(4, 4) = -4.0;
        sfm_mc(4, 5) = -4.0;
        sfm_mc(5, 1) = 4.0;
        sfm_mc(5, 3) = -4.0;
        sfm_mc(5, 5) = -4.0;

        // inverse coordinate transform 
        // [1, tau, tau_c, tau^2, tau_c^2, tau*tau_c] 
        // to [1, x, y, x^2, y^2, x*y]
        il::StaticArray2D<std::complex<double>, 3, 3> cq{0.0};
        for (int j = 0; j <= 1; ++j) {
            for (int k = 0; k <= 1; ++k) {
                cq(j, k) = tau_2_mc(j, k) * tau_2_mc(j, k);
            }
            cq(j, 2) = 2.0 * tau_2_mc(j, 0) * tau_2_mc(j, 1);
            cq(2, j) = tau_2_mc(0, j) * tau_2_mc(1, j);
        }
        cq(2, 2) = tau_2_mc(0, 0) * tau_2_mc(1, 1) + 
                tau_2_mc(1, 0) * tau_2_mc(0, 1);

        il::StaticArray2D<std::complex<double>, 6, 6> tau_sq_2_mc{0.0};
        tau_sq_2_mc(0, 0) = 1.0;
        for (int j = 0; j <= 1; ++j) {
            for (int k = 0; k <= 1; ++k) {
                tau_sq_2_mc(j + 1, k + 1) = tau_2_mc(j, k);
            }
        }
        for (int j = 0; j <= 2; ++j) {
            for (int k = 0; k <= 2; ++k) {
                tau_sq_2_mc(j + 3, k + 3) = cq(j, k);
            }
        }
        // assembly of sfm
        sfm = il::dot(sfm_mc, tau_sq_2_mc);
        return sfm;
    }

    il::StaticArray2D<std::complex<double>, 6, 6> make_el_sfm_nonuniform
            (const il::StaticArray2D<double, 3, 3> &el_vert,
             const il::StaticArray<double, 3> &vertex_wts,
             il::io_t, il::StaticArray2D<double, 3, 3> &r_tensor) {
// This function calculates the basis (shape) functions' 
// coefficients (rows of sfm) for a triangular boundary 
// element with 2nd order polynomial approximation of unknowns
// in terms of complex (tau, conj(tau)) representation of 
// local element's coordinates with non-trivial edge partitioning 
// defined by "weights" vertex_wts

        double p12 = vertex_wts[0] / vertex_wts[1],
                p13 = vertex_wts[0] / vertex_wts[2],
                p23 = vertex_wts[1] / vertex_wts[2],
                c122 = p12 + 1.0,     // (vertex_wts[0]+w(2))/vertex_wts[1];
                c121 = 1.0 / p12 + 1.0,    // (vertex_wts[0]+w(2))/vertex_wts[0];
                c12q = c121 + c122,
                c233 = p23 + 1.0,     // (vertex_wts[1]+w(3))/vertex_wts[2];
                c232 = 1.0 / p23 + 1.0,    // (vertex_wts[1]+w(3))/vertex_wts[1];
                c23q = c232 + c233,
                c133 = p13 + 1.0,     // (vertex_wts[0]+w(3))/vertex_wts[2];
                c131 = 1.0 / p13 + 1.0,    // (vertex_wts[0]+w(3))/vertex_wts[0];
                c13q = c131 + c133;

        r_tensor = make_el_r_tensor(el_vert);
        // tau_2_mc defines inverse coordinate transform 
        // [tau, conj(tau)] to [x,y] (see make_el_tau_2_mc)
        il::StaticArray2D<std::complex<double>, 2, 2> tau_2_mc = 
                make_el_tau_2_mc(el_vert, r_tensor);
        il::StaticArray2D<std::complex<double>, 6, 6> sfm{0.0}, sfm_mc{0.0};

        // coefficients of shape functions (rows) 
        // for master element (0<=x,y<=1); ~[1, x, y, x^2, y^2, x*y]
        sfm_mc(0, 0) = 1.0;
        sfm_mc(0, 1) = -p12 - 2.0;
        sfm_mc(0, 2) = -p13 - 2.0;
        sfm_mc(0, 3) = c122;
        sfm_mc(0, 4) = c133;
        sfm_mc(0, 5) = p13 + p12 + 2.0;
        sfm_mc(1, 1) = -1.0 / p12;
        sfm_mc(1, 3) = c121;
        sfm_mc(1, 5) = 1.0 / p12 - p23;
        sfm_mc(2, 2) = -1.0 / p13;
        sfm_mc(2, 4) = c131;
        sfm_mc(2, 5) = 1.0 / p13 - 1.0 / p23;
        sfm_mc(3, 5) = c23q;
        sfm_mc(4, 2) = c13q;
        sfm_mc(4, 4) = -c13q;
        sfm_mc(4, 5) = -c13q;
        sfm_mc(5, 1) = c12q;
        sfm_mc(5, 3) = -c12q;
        sfm_mc(5, 5) = -c12q;

        // inverse coordinate transform 
        // [1, tau, tau_c, tau^2, tau_c^2, tau*tau_c] 
        // to [1, x, y, x^2, y^2, x*y]
        il::StaticArray2D<std::complex<double>, 3, 3> cq{0.0};
        for (int j = 0; j <= 1; ++j) {
            for (int k = 0; k <= 1; ++k) {
                cq(j, k) = tau_2_mc(j, k) * tau_2_mc(j, k);
            }
            cq(j, 2) = 2.0 * tau_2_mc(j, 0) * tau_2_mc(j, 1);
            cq(2, j) = tau_2_mc(0, j) * tau_2_mc(1, j);
        }
        cq(2, 2) = tau_2_mc(0, 0) * tau_2_mc(1, 1) + 
                tau_2_mc(1, 0) * tau_2_mc(0, 1);

        il::StaticArray2D<std::complex<double>, 6, 6> tau_sq_2_mc{0.0};
        tau_sq_2_mc(0, 0) = 1.0;
        for (int j = 0; j <= 1; ++j) {
            for (int k = 0; k <= 1; ++k) {
                tau_sq_2_mc(j + 1, k + 1) = tau_2_mc(j, k);
            }
        }
        for (int j = 0; j <= 2; ++j) {
            for (int k = 0; k <= 2; ++k) {
                tau_sq_2_mc(j + 3, k + 3) = cq(j, k);
            }
        }
        // assembly of sfm
        sfm = il::dot(sfm_mc, tau_sq_2_mc);
        return sfm;
    };

    //il::StaticArray2D<std::complex<double>, 6, 6> make_el_sfm_beta
    // (const il::StaticArray2D<double, 3, 3> &el_vert,
    // const il::StaticArray<double,3> &vertex_wts
    // double beta,
    // il::io_t, il::StaticArray2D<double, 3, 3> &r_tensor) {
// This function calculates the basis (shape) functions'
// coefficients (rows of sfm) for a triangular boundary
// element with 2nd order polynomial approximation of unknowns
// in terms of complex (tau, conj(tau)) representation of
// local element's coordinates with nodes' offset to the
// centroid (e.g. at collocation points) defined by beta
// and non-trivial edge partitioning defined by
// "weights" vertex_wts
    //};

    il::StaticArray2D<std::complex<double>, 6, 6> shift_el_sfm
            (std::complex<double> z) {
        // "shifted" sfm from z, tau[m], and local sfm
        std::complex<double> zc = std::conj(z);
        il::StaticArray2D<std::complex<double>, 6, 6> shift_2_z{0.0};
        shift_2_z(0, 0) = 1.0;
        shift_2_z(1, 0) = z;
        shift_2_z(1, 1) = 1.0;
        shift_2_z(2, 0) = zc;
        shift_2_z(2, 2) = 1.0;
        shift_2_z(3, 0) = z * z;
        shift_2_z(3, 1) = 2.0 * z;
        shift_2_z(3, 3) = 1.0;
        shift_2_z(4, 0) = zc * zc;
        shift_2_z(4, 2) = 2.0 * zc;
        shift_2_z(4, 4) = 1.0;
        shift_2_z(5, 0) = z * zc;
        shift_2_z(5, 1) = zc;
        shift_2_z(5, 2) = z;
        shift_2_z(5, 5) = 1.0;
        return shift_2_z;
    };

// Collocation points

    il::StaticArray<il::StaticArray<double, 3>, 6> el_cp_uniform
            (const il::StaticArray2D<double, 3, 3> &el_vert, double beta) {
// This function calculates the coordinates
// of the collocation points on a triangular boundary element
// with 2nd order polynomial approximation of unknowns
// and trivial (middle) edge partitioning;
// offset of the points to the centroid is defined by beta;
// returns the same as el_cp_nonuniform(el_vert, {1.0, 1.0, 1.0}, beta)
        il::StaticArray<il::StaticArray<double, 3>, 6> coll_pt_crd;
        il::StaticArray<double, 3> el_centroid{0.0};

        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                el_centroid[j] += el_vert(j, k) / 3.0;
            }
        }
        for (int n = 0; n < 3; ++n) {
            int m = (n + 1) % 3;
            int l = (m + 1) % 3; // the edge across the (n-3)-th node
            for (int j = 0; j < 3; ++j) {
                (coll_pt_crd[n])[j] =
                        (1.0 - beta) * el_vert(j, n) +
                        beta * el_centroid[j];
                (coll_pt_crd[n + 3])[j] =
                        0.5 * (1.0 - beta) *
                        (el_vert(j, m) + el_vert(j, l)) +
                        beta * el_centroid[j];
            }
        }
        return coll_pt_crd;
    };

    il::StaticArray<il::StaticArray<double, 3>, 6> el_cp_nonuniform
            (const il::StaticArray2D<double, 3, 3> &el_vert,
             const il::StaticArray<double, 3> &vertex_wts,
             double beta) {
// This function calculates the coordinates
// of the collocation points on a triangular boundary element
// with 2nd order polynomial approximation of unknowns
// and non-trivial (middle) edge partitioning;
// offset of the points to the centroid is defined by beta;
// returns the same as el_cp_nonuniform(el_vert, {1.0, 1.0, 1.0}, beta)
        il::StaticArray<il::StaticArray<double, 3>, 6> coll_pt_crd;
        il::StaticArray<double, 3> el_centroid{0.0};

        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                el_centroid[j] += el_vert(j, k) / 3.0;
            }
        }
        for (int n = 0; n < 3; ++n) {
            int m = (n + 1) % 3;
            int l = (m + 1) % 3; // the edge across the (n-3)-th node
            for (int j = 0; j < 3; ++j) {
                (coll_pt_crd[n])[j] =
                        (1.0 - beta) * el_vert(j, n) + beta * el_centroid[j];
                (coll_pt_crd[n + 3])[j] =
                        (1.0 - beta) *
                                (vertex_wts[m] * el_vert(j, m) +
                                        vertex_wts[l] * el_vert(j, l)) /
                        (vertex_wts[m] + vertex_wts[l]) +
                                beta * el_centroid[j];
            }
        }
        return coll_pt_crd;
    };

}