#include <igl/per_vertex_normals.h>
#include <igl/principal_curvature.h>
#include <igl/avg_edge_length.h>
#include <igl/massmatrix.h>
#include <igl/adjacency_list.h>
#include <igl/per_face_normals.h>
#include <igl/barycenter.h>
#include <igl/pinv.h>
#include <igl/edges.h>
#include <Eigen/SparseCore>
#include <igl/adjacency_list.h>
#include <igl/adjacency_matrix.h>
#include <igl/per_face_normals.h>
#include <igl/per_vertex_normals.h>
#include <igl/avg_edge_length.h>
#include <igl/edge_flaps.h>
#include <igl/unique_edge_map.h>
#include <igl/vertex_triangle_adjacency.h>
#include <igl/principal_curvature.h>
#include <igl/collapse_edge.h>
#include <igl/is_edge_manifold.h>
#include <igl/C_STR.h>
#include <igl/circulation.h>
#include <igl/decimate.h>
#include <igl/shortest_edge_and_midpoint.h>
#include <igl/infinite_cost_stopping_condition.h>
using namespace std;

void collapse_edges(Eigen::MatrixXd & V,Eigen::MatrixXi & F, Eigen::VectorXi & feature, Eigen::VectorXd & high, Eigen::VectorXd & low){
        using namespace Eigen;
    MatrixXi E,uE,EI,EF;
    VectorXi EMAP,I,J;
    VectorXd data;
    Eigen::MatrixXd U;
    Eigen::MatrixXi G;
    // VectorXd p;
    std::vector<std::vector<int>> uE2E;
    std::vector<std::vector<int>> vertex_face_adjacency;
    std::vector<int> small_edges;
    int e1,e2,f1,f2,e;
    int n = V.rows();
    
    
    int num_feature = feature.size();
    std::vector<std::vector<int>> A;
    igl::adjacency_list(F,A);
    
    std::vector<bool> is_feature_vertex;
    is_feature_vertex.resize(n);
    
    for (int s = 0; s < num_feature; s++) {
        is_feature_vertex[feature(s)] = true;
    }
    
    //igl::is_edge_manifold(F);
    
    std::function<bool(
                       const Eigen::MatrixXd &,
                       const Eigen::MatrixXi &,
                       const Eigen::MatrixXi &,
                       const Eigen::VectorXi &,
                       const Eigen::MatrixXi &,
                       const Eigen::MatrixXi &,
                       const igl::min_heap< std::tuple<double,int,int> > &,
                       const Eigen::VectorXi &,
                       const Eigen::MatrixXd &,
                       const int,
                       const int,
                       const int,
                       const int,
                       const int)>  stopping_condition;
    
    std::function<void(
                       const int,
                       const Eigen::MatrixXd &,
                       const Eigen::MatrixXi &,
                       const Eigen::MatrixXi &,
                       const Eigen::VectorXi &,
                       const Eigen::MatrixXi &,
                       const Eigen::MatrixXi &,
                       double &,
                       Eigen::RowVectorXd &)> shortest_edge_and_midpoint_lambda = [&A,&feature,&low,&high,&is_feature_vertex](
                                                                 const int e,
                                                                 const Eigen::MatrixXd & V,
                                                                 const Eigen::MatrixXi & F,
                                                                 const Eigen::MatrixXi & E,
                                                                 const Eigen::VectorXi & EMAP,
                                                                 const Eigen::MatrixXi & EF,
                                                                 const Eigen::MatrixXi & EI,
                                                                 double & cost,
                                                                 Eigen::RowVectorXd & p)->void{
        igl::shortest_edge_and_midpoint(e,V,F,E,EMAP,EF,EI,cost,p);
        if (is_feature_vertex[E(e,0)] || is_feature_vertex[E(e,1)] ) {
            cost = std::numeric_limits<double>::infinity();
            return;
        }
        if ( (V.row(E(e,0))-V.row(E(e,1))).norm() > ((low(E(e,0))+low(E(e,1)))/2) ) {
            cost = std::numeric_limits<double>::infinity();
            return;
        }
        for(int i = 0; i < A[E(e,1)].size(); i++){
            if((V.row(A[E(e,1)][i])-p).norm() > high(E(e,1))){
                cost = std::numeric_limits<double>::infinity();
                return;
            }
        }
        for(int r = 0; r < A[E(e,0)].size(); r++){
            if((V.row(A[E(e,0)][r])-p).norm() > high(E(e,0))){
                cost = std::numeric_limits<double>::infinity();
                return;
            }
        }
        //std::cout << "Mathing..." << std::endl;
        // consider both directions to circulate
        for(int direction = 0;direction<2;direction++)
        {
            // consider each face
            for(const int f : igl::circulation(e,direction,EMAP,EF,EI))
            {
                if (f < 0) {//?????
                    cost = std::numeric_limits<double>::infinity();
                    return;
                }
                //std::cout << f << std::endl;
                if( f == 0 || f ==  igl::circulation(e,direction,EMAP,EF,EI).size()-1)
                {
                    
                    // skip
                    continue;
                }
                // Grab the three corners of the face
                Eigen::RowVector3d p_before[3], p_after[3];
                for(int c = 0;c<3;c++)
                {
                    // vertex index
//                    std::cout << e << std::endl;
//                    std::cout << f << std::endl;
//                    std::cout << c << std::endl;
                    const int v = F(f,c);
                    if( v == E(e,0) || v == E(e,1))
                    {
                        p_after[c] = p;
                    }else
                    {
                        p_after[c] = V.row(v);
                    }
                    p_before[c] = V.row(v);
                }
                const Eigen::RowVector3d n_before =
                ((p_before[1]- p_before[0]).cross(p_before[2]- p_before[0])).normalized();
                const Eigen::RowVector3d n_after =
                ((p_after[1]- p_after[0]).cross(p_after[2]- p_after[0])).normalized();
                if( n_before.dot(n_after) < n_after.norm()/2 )
                   {
                       cost = std::numeric_limits<double>::infinity();
                   }
                   }
                   }
        //std::cout << "Mathed!" << std::endl;
        
        
        };
    
    igl::infinite_cost_stopping_condition(shortest_edge_and_midpoint_lambda,stopping_condition);
    

     
    //std::cout << "??" << std::endl;
    igl::decimate(V,F,shortest_edge_and_midpoint_lambda,stopping_condition,U,G,J,I);
    //std::cout << "!!" << std::endl;

    Eigen::VectorXd high_new,low_new;
    Eigen::VectorXi feature_new;
    feature_new.resize(num_feature);
    high_new.resize(U.rows());
    low_new.resize(U.rows());
    int j = 0;
    for (int s = 0; s<U.rows(); s++) {
        high_new(s) = high(I(s));
        low_new(s) = low(I(s));
        if (is_feature_vertex[I(s)]) {
            feature_new(j) = s;
            j = j+1;
        }
    }
    
    // PLACEHOLDER
    
    V = U;
    F = G;
    high = high_new;
    low = low_new;
    feature = feature_new;
    
    
    
    
    
}


// g++ -I/usr/local/libigl/external/eigen -I/usr/local/libigl/include -std=c++11 -framework Accelerate main.cpp remesh_botsch.cpp -o main

