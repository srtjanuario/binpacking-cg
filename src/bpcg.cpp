#define MAX_ITER 100

#include <ilcplex/ilocplex.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <fstream>
#include <algorithm>
using namespace std;

struct Combination
{
   // A item set is a vector of pairs of <id,weight>
   vector<pair<int, int>> itemSet;

   // Weight of the
   int totalWeight;
};

int main(int argc, char **argv)
{
   if (argc < 2)
   {
      cout << "I need a input" << endl;
      return 0;
   }
   ifstream arquivo(argv[1], ios::in);

   // Number of itens
   int n;
   arquivo >> n;

   // Capacity of each bin
   int capaticy;
   arquivo >> capaticy;

   // a item is a pair of <id,weight>
   vector<pair<int, int>> item;

   int w;
   for (int i = 0; i < n; i++)
   {
      int w;
      arquivo >> w;
      item.push_back(make_pair(i, w));
   }

   // Vector of all combinations of itens
   vector<Combination> lambda;

   // Insert the first n itens in lambda
   for (int i = 0; i < n; i++)
   {
      // a item is a pair of <id,weight>
      if (item[i].second < capaticy)
      {
         Combination b;
         b.itemSet.push_back(item[i]);
         b.totalWeight = item[i].second;
         lambda.push_back(b);
      }
   }

   // for (int i = 0; i < lambda.size(); i++)
   // {
   //    for (auto j : item)
   //    {
   //       if (lambda[i].totalWeight + j.second <= capaticy)
   //       {
   //          auto it = find(lambda[i].itemSet.begin(), lambda[i].itemSet.end(), j);
   //          if (it == lambda[i].itemSet.end())
   //          {
   //             if (j.first > lambda[i].itemSet.back().first)
   //             {
   //                Combination c = lambda[i];
   //                c.itemSet.push_back(j);
   //                c.totalWeight += j.second;
   //                lambda.push_back(c);
   //             }
   //          }
   //       }
   //    }
   // }

   int Q = lambda.size();

   for (auto i : lambda)
   {
      for (auto j : i.itemSet)
         cout << j.second << " ";
      cout << "- " << i.totalWeight << endl;
   }

   // Set an environment
   IloEnv env;
   env.setName("bin packing");

   // Create a model
   IloModel bpcg(env);

   bpcg.setName("A Column Generation for the Bin Packing Problem");
   try
   {
      // Start with one variable for each item combination
      IloBoolVarArray L(env, Q);
      for (int i = 0; i < n; i++)
      {
         string name = "L_" + to_string(i+1);
         L[i].setName(name.c_str());
      }

      // Objective is to minimize the sum of combinations
      IloExpr obj(env);
      for (int i = 0; i < n; i++)
         obj += L[i];
      bpcg.add(IloMinimize(env, obj));

      // A constraint for each item
      for (auto i : item)
      {
         IloExpr expr(env);
         for (IloInt j = 0; j < n; j++){
            auto it = find(lambda[j].itemSet.begin(), lambda[j].itemSet.end(), i);
            if (it != lambda[j].itemSet.end())
               expr += L[j];
         }
         IloConstraint c(expr == 1);
         string name = "Item_" + to_string(i.first);
         c.setName(name.c_str());
         bpcg.add(c);
      }

      IloCplex cplex(bpcg);
      // Export the LP model to a txt file to check correctness
      cplex.exportModel("model.lp");

      // Tolerance
      // IloNum tol = cplex.getParam(IloCplex::EpInt);

      bool solved = cplex.solve();

      if (solved)
         env.out() << "Optimal value "<< cplex.getObjValue() << endl;

      //          IloNumArray2 sol(env, n);
      //          for (IloInt i = 0; i < n; i++) {
      //              sol[i] = IloNumArray(env,n);
      //              cplex.getValues(sol[i], x[i]);
      //          }
      //          IloBoolArray seen(env);
      //          IloInt length = checkTour(sol, seen, tol);

      //          if ( length < n ) {
      //             IloExpr clique(env);
      //             for (int i = 0; i < n; i++) {
      //                if ( seen[i] ) {
      //                   for (int j = i+1; j < n; j++) {
      //                      if ( seen[j] ) clique += x[j][i];
      //                   }
      //                }
      //             }
      //             cerr << cplex.getValue(clique) << " <= " << length-1 << endl;
      //          }

      //          // assert (length == n);

      // #ifdef FULLTEST
      //       assert(cplex.getImpl()->isConsistent());
      //       assert(cpxtest.getImpl()->isConsistent());
      //       assert(cplex.getStatus() == IloAlgorithm::Optimal);
      //       assert(fabs(cplex.getObjValue() - 11461.0) < 1e-6);
      //       assert(cutCalled);
      //       env.out() << "Test completed successfully" << endl;
      // #endif

      //       // sec.end();

      //       for (IloInt i = 0; i < n; i++)
      //          dist[i].end();
      //       dist.end();
      //    }
      //    catch (const IloException &e)
      //    {
      //       cerr << "Exception caught: " << e << endl;
      // #ifdef FULLTEST
      //       assert(0);
      // #endif
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
