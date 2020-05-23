// -------------------------------------------------------------- -*- C++ -*-
// File: examples/test/ilocplex/tsp.cpp
// Version 12.5
// --------------------------------------------------------------------------
// Licensed Materials - Property of IBM
// 5725-A06 5725-A29 5724-Y48 5724-Y49 5724-Y54 5724-Y55 5655-Y21
// Copyright IBM Corporation 2000, 2012. All Rights Reserved.
//
// US Government Users Restricted Rights - Use, duplication or
// disclosure restricted by GSA ADP Schedule Contract with
// IBM Corp.
// --------------------------------------------------------------------------
//
//
// tsp.cpp -- Solves a simple traveling salesman problem.
//
// This example solves a small traveling salesman problem
// as a MIP model containing two sets of constraints:
//   --- degree:  each city is visited once
//                (exactly two edges are incident to each
//                 node in the graph representation)
//   --- subtour: a tour contains no subtours
//                (given a proper subset of the nodes of size
//                 L, the set of edges between pairs of nodes
//                 in this set is of size no more than L-1)
// All of the degree constraints are included in the initial
// MIP, while subtour constraints are treated as lazy constraints,
// and are added dynamically through the cut callback.

#define MAX_ITER 100

#include <ilcplex/ilocplex.h>
#include "data.h"
#include "NodeInfo.h"
#include <vector>
#include <cmath>
#include <cstdlib>
using namespace std;


void displayPathtoPrincess(int n, vector <string> grid){
    //your logic here
    unsigned x,y;
    bool found = false;
    int posx,posy;
    x = y = 0;
    for(x = 0; x < grid.size() && !found; x++)
        for(y = 0; y < grid.size() && !found; y++)
            if(grid[x][y] == 'p'){
                found = true;
                posx = x;
                posy = y;
                break;
            }
    if( posx > 1)
        cout<<"RIGHT\n";
    else
        cout<<"LEFT\n";
    if( posy > 1)
        cout<<"DOWN\n";
    else
        cout<<"UP\n";
                
}

#ifdef FULLTEST
#include <assert.h>
IloBool cutCalled = IloFalse;
#endif

ILOSTLBEGIN

typedef IloArray<IloBoolVarArray> Edges;

IloInt checkTour(IloNumArray2 sol, IloBoolArray seen, IloNum tol)
{
   IloInt j, n = sol.getSize();
   IloInt last = -1;
   IloInt length = 0;
   IloInt current = 0;
   seen.clear();
   seen.add(n, 0.0);

   // Search for a subtour if sol[] is integer feasible

   while (seen[current] == 0)
   {
      cout<<current<<" ";
      length++;
      seen[current] = length;
      for (j = 0; j < current; j++)
      {
         if (j != last && sol[current][j] >= 1.0 - tol)
            break;
      }
      if (j == current)
      {
         for (j = current + 1; j < n; j++)
         {
            if (j != last && sol[j][current] >= 1.0 - tol)
               break;
         }
      }
      if (j == n)
         return (n + 1);
      last = current;
      current = j;
   }
   cout<<endl;
   return (length);
}

ILOUSERCUTCALLBACK2(MaxBack, Edges, x, IloNum, tol)
{
   NodeInfo *data = dynamic_cast<NodeInfo *>(getNodeData());
     if (!data)
    {
        if (NodeInfo::rootData == NULL)
        {
            NodeInfo::initRootData();
        }
        data = NodeInfo::rootData;
    }

   if (data->getIterations() >= MAX_ITER)
      return;

   IloEnv env = getEnv();
   IloInt n = x.getSize();

   IloNumArray2 sol(env, n);
   for (IloInt i = 0; i < n; i++)
   {
      sol[i] = IloNumArray(env, n);
      getValues(sol[i], x[i]);
   }

   for (IloInt i = 0; i < n; i++)
      for (IloInt j = 0; j < i; j++)
         sol[j][i] = sol[i][j];

   vector<int> S;
   vector<int> Smin;
   vector<bool> seen(n);
   vector<double> b(n);
   fill(seen.begin(), seen.end(), false);

   int v = rand()%n;
   S.push_back(v);
   seen[v] = true;
   b[v] = -numeric_limits<double>::infinity();

   double Cutmin;
   double Cutval;
   Cutmin = 0.0;
   for (int i = 0; i < n; i++)
   {
      if (!seen[i] && v != i)
      {
         b[i] = sol[v][i];
         Cutmin += b[i];
      }
   }

   // if(Cutmin > 0){
   //    int a = 0;
   //    for(auto i:b)
   //       cout<<"b["<<a++<<"] = "<<i<<endl;
   //    exit(0);
   // }

   Smin = S;
   Cutval = Cutmin;

   while (S.size() < n)
   {
      // Choose v not in S of maximum max-back value b(v)
      double maxb = -numeric_limits<double>::infinity();
      for (int i = 0; i < n; i++)
         if (!seen[i])
            if (maxb < b[i])
            {
               maxb = b[i];
               v = i;
            }

      S.push_back(v);
      seen[v] = true;
      Cutval = Cutval + 2 - 2 * b[v];

      for (int t = 0; t < n; t++)
      {
         if (!seen[t] && v != t)
            b[t] = b[t] + sol[v][t];
      }
      if (Cutval < Cutmin)
      {
         Cutmin = Cutval;
         Smin = S;
      }
   }

   if (Cutmin < 2-tol)
   {
      fill(seen.begin(), seen.end(), false);
      vector<int> v = Smin;
      for (int i = 0; i < v.size(); i++)
         seen[v[i]] = true;
      vector<int> y;
      for (int i = 0; i < n; i++)
         if (!seen[i])
            y.push_back(i);
      if (v.size() > 0 && y.size() > 0)
      {
         IloExpr expr1(env);
         for (int i = 0; i < v.size(); i++)
            for (int j = 0; j < y.size(); j++)
               expr1 += x[v[i]][y[j]] + x[y[j]][v[i]];
         // cout << "Adding cut " << expr1 << " >= 2 " << endl;
         add(expr1 >= 2).end();
         expr1.end();
      }
   }

   // return Smin;

   // IloInt const nbLocations = used.getSize();
   // IloInt const nbClients = supply.getSize();

   // // For each j and c check whether in the current solution (obtained by
   // // calls to getValue()) we have supply[c][j]>used[j]. If so, then we have
   // // found a violated constraint and add it as a cut.
   // for (IloInt j = 0; j < nbLocations; ++j) {
   //    for (IloInt c = 0; c < nbClients; ++c) {
   //       IloNum const s = getValue(supply[c][j]);
   //       IloNum const o = getValue(used[j]);
   //       if ( s > o + EPS) {
   //          cout << "Adding: " << supply[c][j].getName() << " <= "
   //               << used[j].getName() << " [" << s << " > " << o << "]" << endl;
   //          add(supply[c][j] <= used[j]).end();
   //       }
   //    }
   // }
}

ILOLAZYCONSTRAINTCALLBACK2(SubtourEliminationCallback, Edges, x, IloNum, tol)
{

   IloEnv env = getEnv();
   IloInt n = x.getSize();

   IloNumArray2 sol(env, n);
   for (IloInt i = 0; i < n; i++)
   {
      sol[i] = IloNumArray(env, n);
      getValues(sol[i], x[i]);
   }

   for (IloInt i = 0; i < n; i++)
      for (IloInt j = 0; j < i; j++)
         sol[j][i] = sol[i][j];

   // Declares a boolean vector of size n with false
   vector<bool> seen(n,false);

   // An array to store the subtours
   IloNumArray tour(env, n);

   IloInt i, node, len, start;

   // Vector os positions
   vector<pair<int, int>> p;

   for (i = 0; i < n; i++)
      seen[i] = false;

   start = 0;
   node = 0;

   // Start from position 0 in the tour
   while (start < n)
   {
      // Find a node that has not been seen
      for (node = 0; node < n; node++)
         if (!seen[node])
            break;
      // Did you see every node? Time to break
      if (node == n)
         break;

      // Start with lenght 0 and build a subtour
      for (len = 0; len < n; len++)
      {
         // insert the unseen node in the tour
         tour[start + len] = node;

         //Be honest if you saw that node
         seen[node] = true;

         // Check nodes neighbors
         for (i = 0; i < n; i++)
         {
            // Is it connected to someone?
            // First time you see it? ...
            if (sol[node][i] >= 1.0 - tol && !seen[i])
            {
               // ... better catch that guy
               node = i;
               break;
            }
         }

         // Oh man, you could not find a neighbhor? It seens that you closed the loop
         if (i == n)
         {
            // In this case, increase the size of the lenght
            len++;
            pair<int, int> pos(start, len);
            p.push_back(pos);
            //Start a new subtour
            start += len;
            break;
         }
      }
   }

   // Create and add subtour constraint ---
   // No more than 'length-1' edges between members of the subtour
   if (p.size() > 1)
   {
      for (i = 0; i < p.size(); i++)
      {
         IloExpr expr1(env);
         for (int a = p[i].first; a < p[i].first + p[i].second; a++)
            for (int b = a + 1; b < p[i].first + p[i].second; b++)
               expr1 += x[tour[a]][tour[b]] + x[tour[b]][tour[a]];
         // cout << "Adding lazy constraint " << expr1 << " <= " << p[i].second - 1 << endl;
         add(expr1 <= p[i].second - 1).end();
         expr1.end();
      }
   }

   for (IloInt i = 0; i < n; i++)
      for (IloInt j = 0; j < i; j++)
         sol[j][i] = sol[i][j];

   vector<int> S;
   vector<int> Smin;
   vector<double> b(n);
   fill(seen.begin(), seen.end(), false);

   int v = rand()%n;
   S.push_back(v);
   seen[v] = true;
   b[v] = -numeric_limits<double>::infinity();

   double Cutmin;
   double Cutval;
   Cutmin = 0.0;
   for (int i = 0; i < n; i++)
   {
      if (!seen[i] && v != i)
      {
         b[i] = sol[v][i];
         Cutmin += b[i];
      }
   }

   Smin = S;
   Cutval = Cutmin;

   while (S.size() < n)
   {
      // Choose v not in S of maximum max-back value b(v)
      double maxb = -numeric_limits<double>::infinity();
      for (int i = 0; i < n; i++)
         if (!seen[i])
            if (maxb < b[i])
            {
               maxb = b[i];
               v = i;
            }

      S.push_back(v);
      seen[v] = true;
      Cutval = Cutval + 2 - 2 * b[v];

      for (int t = 0; t < n; t++)
      {
         if (!seen[t] && v != t)
            b[t] = b[t] + sol[v][t];
      }
      if (Cutval < Cutmin)
      {
         Cutmin = Cutval;
         Smin = S;
      }
   }

   if (Cutmin < 2-tol)
   {
      fill(seen.begin(), seen.end(), false);
      vector<int> v = Smin;
      for (int i = 0; i < v.size(); i++)
         seen[v[i]] = true;
      vector<int> y;
      for (int i = 0; i < n; i++)
         if (!seen[i])
            y.push_back(i);
      if (v.size() > 0 && y.size() > 0)
      {
         IloExpr expr1(env);
         for (int i = 0; i < v.size(); i++)
            for (int j = 0; j < y.size(); j++)
               expr1 += x[v[i]][y[j]] + x[y[j]][v[i]];
         // cout << "Adding cut " << expr1 << " >= 2 " << endl;
         add(expr1 >= 2).end();
         expr1.end();
      }
   }
   seen.end();
   tour.end();
   for (IloInt i = 0; i < n; i++)
      sol[i].end();
   sol.end();
}

int main(int argc, char **argv)
{
   Data input(argc, argv[1]);
   input.readData();

   IloInt n = input.getDimension();

   //Environment
   IloEnv env;
   env.setName("Branch and Cut");
   // create model
   IloModel tsp(env);
   tsp.setName("Traveling Tournament Problem Model");
   try
   {
      // A matrix for storing the input distances
      IloNumArray2 dist(env, n);

      // You are allocating memory. Remmember to free it before leaving
      for (IloInt i = 0; i < n; i++)
         dist[i] = IloNumArray(env, n);

      for (IloInt i = 0; i < n; i++)
         for (IloInt j = 0; j < n; j++)
            dist[i][j] = input.getDistance(i, j);

      // Create variables x[i][j] for all 0 <= j < i < n representing the
      // edge between cities i and j.  A setting of 1 indicates that the edge
      // is part of the tour.
      Edges x(env, n);
      for (IloInt i = 0; i < n; i++)
      {
         x[i] = IloBoolVarArray(env, n);
         for (IloInt j = 0; j < n; j++)
         {
            string name = "x_" + to_string(i) + "_" + to_string(j);
            x[i][j].setName(name.c_str());
         }
      }

      // Objective is to minimize the sum of edge weights for traveled edges
      IloExpr obj(env);
      for (IloInt i = 0; i < n; i++)
      {
         obj += IloScalProd(x[i], dist[i]);
      }
      tsp.add(IloMinimize(env, obj));

      // degree constraints --- exactly two traveled edges incident to each node
      for (IloInt i = 0; i < n; i++)
      {
         IloExpr expr(env);
         for (IloInt j = 0; j < i; j++)
            expr += x[i][j];
         for (IloInt j = i + 1; j < n; j++)
            expr += x[j][i];
         IloConstraint c(expr == 2);
         c.setName("Degree");
         tsp.add(c);
      }

      IloCplex cplex(tsp);
      // Export the LP model to a txt file to check correctness
      cplex.exportModel("model.lp");

      IloNum tol = cplex.getParam(IloCplex::EpInt);

      cplex.use(SubtourEliminationCallback(env, x, tol));
      cplex.use(MaxBack(env, x, tol));
      cplex.setParam(IloCplex::PreInd, IloFalse);

      bool solved = cplex.solve();

      if (solved)
         env.out() << "Optimal tour length "
                   << cplex.getObjValue() << endl;

         IloNumArray2 sol(env, n);
         for (IloInt i = 0; i < n; i++) {
             sol[i] = IloNumArray(env,n);
             cplex.getValues(sol[i], x[i]);
         }
         IloBoolArray seen(env);
         IloInt length = checkTour(sol, seen, tol);

         if ( length < n ) {
            IloExpr clique(env);
            for (int i = 0; i < n; i++) {
               if ( seen[i] ) {
                  for (int j = i+1; j < n; j++) {
                     if ( seen[j] ) clique += x[j][i];
                  }
               }
            }
            cerr << cplex.getValue(clique) << " <= " << length-1 << endl;
         }

         // assert (length == n);

#ifdef FULLTEST
      assert(cplex.getImpl()->isConsistent());
      assert(cpxtest.getImpl()->isConsistent());
      assert(cplex.getStatus() == IloAlgorithm::Optimal);
      assert(fabs(cplex.getObjValue() - 11461.0) < 1e-6);
      assert(cutCalled);
      env.out() << "Test completed successfully" << endl;
#endif

      // sec.end();

      for (IloInt i = 0; i < n; i++)
         dist[i].end();
      dist.end();
   }
   catch (const IloException &e)
   {
      cerr << "Exception caught: " << e << endl;
#ifdef FULLTEST
      assert(0);
#endif
   }
   catch (...)
   {
      cerr << "Unknown exception caught!" << endl;
#ifdef FULLTEST
      assert(0);
#endif
   }

   env.end();
   return 0;
}
