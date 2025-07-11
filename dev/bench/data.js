window.BENCHMARK_DATA = {
  "lastUpdate": 1752252651848,
  "repoUrl": "https://github.com/buddy-computer/loredb",
  "entries": {
    "Benchmark": [
      {
        "commit": {
          "author": {
            "name": "buddy-computer",
            "username": "buddy-computer"
          },
          "committer": {
            "name": "buddy-computer",
            "username": "buddy-computer"
          },
          "id": "8c3f263de688f5e3ed84c03dffed02211e43717a",
          "message": "Enhance README and add Continuous Benchmarking workflow",
          "timestamp": "2025-07-10T16:55:21Z",
          "url": "https://github.com/buddy-computer/loredb/pull/4/commits/8c3f263de688f5e3ed84c03dffed02211e43717a"
        },
        "date": 1752251944303,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "QueryBenchmarkFixture/GetNodeById",
            "value": 2911.5398846966286,
            "unit": "ns/iter",
            "extra": "iterations: 232257\ncpu: 2911.408060898057 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetEdgeById",
            "value": 0,
            "unit": "ns/iter",
            "extra": "iterations: 100000\ncpu: 0 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetNodesByProperty",
            "value": 282676.8825680602,
            "unit": "ns/iter",
            "extra": "iterations: 2461\ncpu: 282635.80333197897 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetEdgesByProperty",
            "value": 12457788.29629608,
            "unit": "ns/iter",
            "extra": "iterations: 54\ncpu: 12456572.666666677 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetAdjacentNodes",
            "value": 19410.283754632866,
            "unit": "ns/iter",
            "extra": "iterations: 36158\ncpu: 19408.368549145413 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetOutgoingEdges",
            "value": 352.1976102840625,
            "unit": "ns/iter",
            "extra": "iterations: 2012122\ncpu: 352.1815063897717 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetIncomingEdges",
            "value": 345.6928527245313,
            "unit": "ns/iter",
            "extra": "iterations: 2012725\ncpu: 345.6601602305333 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/FindShortestPath",
            "value": 2009795.238095187,
            "unit": "ns/iter",
            "extra": "iterations: 399\ncpu: 2009541.7418546337 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/FindPathsWithLength",
            "value": 24419180.666666135,
            "unit": "ns/iter",
            "extra": "iterations: 33\ncpu: 24417517.393939424 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetDocumentBacklinks",
            "value": 342.2299114121438,
            "unit": "ns/iter",
            "extra": "iterations: 2050281\ncpu: 342.201143648115 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetDocumentOutlinks",
            "value": 342.9274091404249,
            "unit": "ns/iter",
            "extra": "iterations: 1962754\ncpu: 342.8918748860025 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/FindRelatedDocuments",
            "value": 19465.87187039719,
            "unit": "ns/iter",
            "extra": "iterations: 35987\ncpu: 19463.14669186089 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/SuggestLinksForDocument",
            "value": 19702.861722151993,
            "unit": "ns/iter",
            "extra": "iterations: 35595\ncpu: 19702.022418879085 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/BatchGetNodes",
            "value": 27714.12242826457,
            "unit": "ns/iter",
            "extra": "iterations: 25615\ncpu: 27711.479250439108 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/BatchGetEdges",
            "value": 24660.3096428822,
            "unit": "ns/iter",
            "extra": "iterations: 28394\ncpu: 24657.519616820486 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/CountNodes",
            "value": 81.71852532608987,
            "unit": "ns/iter",
            "extra": "iterations: 8597182\ncpu: 81.71042639320633 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/CountEdges",
            "value": 81.58114682166268,
            "unit": "ns/iter",
            "extra": "iterations: 8252338\ncpu: 81.57559687933306 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetNodeDegreeStats",
            "value": 159.70831846518092,
            "unit": "ns/iter",
            "extra": "iterations: 4388574\ncpu: 159.69266531679747 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/1",
            "value": 18376.613007879118,
            "unit": "ns/iter",
            "extra": "iterations: 38838\ncpu: 18374.431072660795 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/8",
            "value": 18364.50479619366,
            "unit": "ns/iter",
            "extra": "iterations: 38885\ncpu: 18363.505336247836 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/64",
            "value": 18364.78373729141,
            "unit": "ns/iter",
            "extra": "iterations: 38948\ncpu: 18362.66303789656 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/512",
            "value": 18352.068739560527,
            "unit": "ns/iter",
            "extra": "iterations: 38915\ncpu: 18350.863291789698 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/4096",
            "value": 18364.951321279732,
            "unit": "ns/iter",
            "extra": "iterations: 38826\ncpu: 18363.101220831497 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/32768",
            "value": 18367.794783413912,
            "unit": "ns/iter",
            "extra": "iterations: 38876\ncpu: 18366.47625784555 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/262144",
            "value": 18365.386240255473,
            "unit": "ns/iter",
            "extra": "iterations: 38867\ncpu: 18363.7775233489 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/1000000",
            "value": 18361.535357170626,
            "unit": "ns/iter",
            "extra": "iterations: 38903\ncpu: 18359.998894686913 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/10",
            "value": 183598.05997931893,
            "unit": "ns/iter",
            "extra": "iterations: 3868\ncpu: 183580.83428128125 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/64",
            "value": 1178327.9771614873,
            "unit": "ns/iter",
            "extra": "iterations: 613\ncpu: 1178288.432300164 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/512",
            "value": 9478028.77333326,
            "unit": "ns/iter",
            "extra": "iterations: 75\ncpu: 9477226.320000038 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/4096",
            "value": 76198415.44444247,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 76195358.33333342 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/10000",
            "value": 186630661.75000155,
            "unit": "ns/iter",
            "extra": "iterations: 4\ncpu: 186615583.7499992 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/1",
            "value": 18574.08413461526,
            "unit": "ns/iter",
            "extra": "iterations: 37856\ncpu: 18572.663514370168 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/8",
            "value": 18553.89979647349,
            "unit": "ns/iter",
            "extra": "iterations: 37833\ncpu: 18552.043163375787 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/64",
            "value": 18580.23247290872,
            "unit": "ns/iter",
            "extra": "iterations: 37927\ncpu: 18579.11717246302 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/512",
            "value": 18598.467159039916,
            "unit": "ns/iter",
            "extra": "iterations: 38047\ncpu: 18596.306410492194 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/4096",
            "value": 18627.12227131645,
            "unit": "ns/iter",
            "extra": "iterations: 38022\ncpu: 18625.5182525906 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/32768",
            "value": 18569.812191406818,
            "unit": "ns/iter",
            "extra": "iterations: 38076\ncpu: 18567.525580418012 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/262144",
            "value": 18598.722021470312,
            "unit": "ns/iter",
            "extra": "iterations: 37913\ncpu: 18597.01909635222 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/1000000",
            "value": 18574.77445020117,
            "unit": "ns/iter",
            "extra": "iterations: 37832\ncpu: 18571.96101184165 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeLookup/1000",
            "value": 1930.4876113795956,
            "unit": "ns/iter",
            "extra": "iterations: 364407\ncpu: 1930.3899623223533 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeLookup/4096",
            "value": 1998.4767423098808,
            "unit": "ns/iter",
            "extra": "iterations: 347068\ncpu: 1998.3222653774 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeLookup/32768",
            "value": 2563.8712136634654,
            "unit": "ns/iter",
            "extra": "iterations: 275068\ncpu: 2563.4882101880185 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeLookup/100000",
            "value": 2695.2189249071653,
            "unit": "ns/iter",
            "extra": "iterations: 259066\ncpu: 2695.014220314471 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeLookup/1000",
            "value": 1798.5195622058475,
            "unit": "ns/iter",
            "extra": "iterations: 392696\ncpu: 1798.3187172774926 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeLookup/4096",
            "value": 1884.2390112938153,
            "unit": "ns/iter",
            "extra": "iterations: 369493\ncpu: 1884.1342190514938 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeLookup/32768",
            "value": 2423.1887787685605,
            "unit": "ns/iter",
            "extra": "iterations: 289683\ncpu: 2423.1238560771435 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeLookup/100000",
            "value": 2482.0769511663834,
            "unit": "ns/iter",
            "extra": "iterations: 295759\ncpu: 2481.8749218113585 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexCreation/1000",
            "value": 676.7295581357495,
            "unit": "ns/iter",
            "extra": "iterations: 1031009\ncpu: 680.9626511532887 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexCreation/4096",
            "value": 680.8816850296289,
            "unit": "ns/iter",
            "extra": "iterations: 1018113\ncpu: 685.0305172488156 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexCreation/32768",
            "value": 682.4761830102635,
            "unit": "ns/iter",
            "extra": "iterations: 1013037\ncpu: 686.7639474150218 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexCreation/100000",
            "value": 692.6399839579457,
            "unit": "ns/iter",
            "extra": "iterations: 992242\ncpu: 697.0093223206992 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexLookup/1000",
            "value": 727.979238303956,
            "unit": "ns/iter",
            "extra": "iterations: 921890\ncpu: 727.9415667812852 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexLookup/4096",
            "value": 2717.486187888307,
            "unit": "ns/iter",
            "extra": "iterations: 256152\ncpu: 2712.0704933008724 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexLookup/32768",
            "value": 21786.03131628142,
            "unit": "ns/iter",
            "extra": "iterations: 32060\ncpu: 21784.742358078227 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexLookup/100000",
            "value": 66109.80855089144,
            "unit": "ns/iter",
            "extra": "iterations: 10572\ncpu: 66103.22654180764 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/AdjacencyLookup/1000",
            "value": 43327.59061101878,
            "unit": "ns/iter",
            "extra": "iterations: 16317\ncpu: 43321.50867193811 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/AdjacencyLookup/4096",
            "value": 43540.169116691875,
            "unit": "ns/iter",
            "extra": "iterations: 15374\ncpu: 43535.25029270166 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/AdjacencyLookup/32768",
            "value": 43793.61477293963,
            "unit": "ns/iter",
            "extra": "iterations: 16097\ncpu: 43790.60359073093 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/AdjacencyLookup/100000",
            "value": 42890.8738337208,
            "unit": "ns/iter",
            "extra": "iterations: 16827\ncpu: 42886.68455458381 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/SimpleQuery/1000",
            "value": 81.50675323446629,
            "unit": "ns/iter",
            "extra": "iterations: 8603507\ncpu: 81.50431515892315 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/SimpleQuery/4096",
            "value": 81.74634833051418,
            "unit": "ns/iter",
            "extra": "iterations: 8597369\ncpu: 81.73979818709508 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/SimpleQuery/32768",
            "value": 82.27674883431224,
            "unit": "ns/iter",
            "extra": "iterations: 8508296\ncpu: 82.27086363709029 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/SimpleQuery/100000",
            "value": 83.26801437503838,
            "unit": "ns/iter",
            "extra": "iterations: 8419873\ncpu: 83.26051509327895 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PathFinding/1000",
            "value": 2390603.074733264,
            "unit": "ns/iter",
            "extra": "iterations: 281\ncpu: 2390220.295373656 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PathFinding/4096",
            "value": 2461455.633093512,
            "unit": "ns/iter",
            "extra": "iterations: 278\ncpu: 2461210.194244629 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PathFinding/10000",
            "value": 2395819.4382022186,
            "unit": "ns/iter",
            "extra": "iterations: 267\ncpu: 2395413.2209737278 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/DocumentBacklinks/1000",
            "value": 330.3321984872217,
            "unit": "ns/iter",
            "extra": "iterations: 2117174\ncpu: 330.3079321775147 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/DocumentBacklinks/4096",
            "value": 330.87186055011716,
            "unit": "ns/iter",
            "extra": "iterations: 2112472\ncpu: 330.8523639603244 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/DocumentBacklinks/10000",
            "value": 331.41823508761263,
            "unit": "ns/iter",
            "extra": "iterations: 2116513\ncpu: 331.38570563942443 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/WikiWorkload/iterations:100000",
            "value": 4300.303920000488,
            "unit": "ns/iter",
            "extra": "iterations: 100000\ncpu: 4299.411149999913 ns\nthreads: 1"
          }
        ]
      },
      {
        "commit": {
          "author": {
            "name": "buddy-computer",
            "username": "buddy-computer"
          },
          "committer": {
            "name": "buddy-computer",
            "username": "buddy-computer"
          },
          "id": "1e8eb2d4a60cbefba7cbba07116dd34e5173a3ec",
          "message": "Enhance README and add Continuous Benchmarking workflow",
          "timestamp": "2025-07-11T16:00:34Z",
          "url": "https://github.com/buddy-computer/loredb/pull/4/commits/1e8eb2d4a60cbefba7cbba07116dd34e5173a3ec"
        },
        "date": 1752252651497,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "QueryBenchmarkFixture/GetNodeById",
            "value": 2866.7596486240536,
            "unit": "ns/iter",
            "extra": "iterations: 242703\ncpu: 2866.600458173158 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetEdgeById",
            "value": 0,
            "unit": "ns/iter",
            "extra": "iterations: 100000\ncpu: 0 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetNodesByProperty",
            "value": 281663.4300120951,
            "unit": "ns/iter",
            "extra": "iterations: 2479\ncpu: 281637.20330778556 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetEdgesByProperty",
            "value": 12637717.79629613,
            "unit": "ns/iter",
            "extra": "iterations: 54\ncpu: 12635913.055555543 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetAdjacentNodes",
            "value": 19720.014796788826,
            "unit": "ns/iter",
            "extra": "iterations: 35751\ncpu: 19717.605409638876 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetOutgoingEdges",
            "value": 350.2395817377655,
            "unit": "ns/iter",
            "extra": "iterations: 2017299\ncpu: 350.2131429203106 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetIncomingEdges",
            "value": 361.11722966056374,
            "unit": "ns/iter",
            "extra": "iterations: 2007393\ncpu: 361.0799335257224 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/FindShortestPath",
            "value": 2047658.0382653342,
            "unit": "ns/iter",
            "extra": "iterations: 392\ncpu: 2047495.98979592 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/FindPathsWithLength",
            "value": 24844642.272727687,
            "unit": "ns/iter",
            "extra": "iterations: 33\ncpu: 24842815.454545494 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetDocumentBacklinks",
            "value": 347.05706664710067,
            "unit": "ns/iter",
            "extra": "iterations: 2016607\ncpu: 347.0347945831781 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetDocumentOutlinks",
            "value": 348.2270124604518,
            "unit": "ns/iter",
            "extra": "iterations: 2019431\ncpu: 348.20182714834004 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/FindRelatedDocuments",
            "value": 19707.368613342835,
            "unit": "ns/iter",
            "extra": "iterations: 35856\ncpu: 19704.922913877785 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/SuggestLinksForDocument",
            "value": 20013.20488265065,
            "unit": "ns/iter",
            "extra": "iterations: 34981\ncpu: 20010.94834338634 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/BatchGetNodes",
            "value": 27621.131931932945,
            "unit": "ns/iter",
            "extra": "iterations: 24975\ncpu: 27616.915995995987 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/BatchGetEdges",
            "value": 25650.855283500823,
            "unit": "ns/iter",
            "extra": "iterations: 27813\ncpu: 25647.284866788865 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/CountNodes",
            "value": 81.541082889154,
            "unit": "ns/iter",
            "extra": "iterations: 8512875\ncpu: 81.53425675814599 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/CountEdges",
            "value": 81.7467001085758,
            "unit": "ns/iter",
            "extra": "iterations: 7221677\ncpu: 81.74398370350816 ns\nthreads: 1"
          },
          {
            "name": "QueryBenchmarkFixture/GetNodeDegreeStats",
            "value": 158.0979922608644,
            "unit": "ns/iter",
            "extra": "iterations: 4441065\ncpu: 158.08606989539726 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/1",
            "value": 18581.200308085547,
            "unit": "ns/iter",
            "extra": "iterations: 38301\ncpu: 18578.303020808784 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/8",
            "value": 18582.222806880265,
            "unit": "ns/iter",
            "extra": "iterations: 38199\ncpu: 18579.765438885857 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/64",
            "value": 18578.656081737532,
            "unit": "ns/iter",
            "extra": "iterations: 38073\ncpu: 18576.595855330386 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/512",
            "value": 18647.854171028033,
            "unit": "ns/iter",
            "extra": "iterations: 38216\ncpu: 18646.053171446525 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/4096",
            "value": 18606.47818805122,
            "unit": "ns/iter",
            "extra": "iterations: 38213\ncpu: 18581.76466124079 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/32768",
            "value": 18581.3230165993,
            "unit": "ns/iter",
            "extra": "iterations: 38255\ncpu: 18579.802797019936 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/262144",
            "value": 18560.982552512734,
            "unit": "ns/iter",
            "extra": "iterations: 38229\ncpu: 18558.720317036757 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeCreation/1000000",
            "value": 18629.38841723005,
            "unit": "ns/iter",
            "extra": "iterations: 38281\ncpu: 18609.108408871234 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/10",
            "value": 186049.39853556795,
            "unit": "ns/iter",
            "extra": "iterations: 3824\ncpu: 185926.10852510488 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/64",
            "value": 1192549.4433333485,
            "unit": "ns/iter",
            "extra": "iterations: 600\ncpu: 1192406.3650000012 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/512",
            "value": 9569345.797297353,
            "unit": "ns/iter",
            "extra": "iterations: 74\ncpu: 9568961.81081083 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/4096",
            "value": 76694332.66666678,
            "unit": "ns/iter",
            "extra": "iterations: 9\ncpu: 76687801.44444457 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/BatchNodeCreation/10000",
            "value": 187961329.7500029,
            "unit": "ns/iter",
            "extra": "iterations: 4\ncpu: 187932683.24999878 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/1",
            "value": 18720.416269724825,
            "unit": "ns/iter",
            "extra": "iterations: 37579\ncpu: 18718.71688975232 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/8",
            "value": 18621.951147062682,
            "unit": "ns/iter",
            "extra": "iterations: 37705\ncpu: 18621.130778411272 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/64",
            "value": 18655.716858999138,
            "unit": "ns/iter",
            "extra": "iterations: 37695\ncpu: 18653.514312242973 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/512",
            "value": 18667.489317145497,
            "unit": "ns/iter",
            "extra": "iterations: 37724\ncpu: 18663.486030113447 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/4096",
            "value": 18665.54179443852,
            "unit": "ns/iter",
            "extra": "iterations: 37828\ncpu: 18662.61983187052 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/32768",
            "value": 18656.419730440706,
            "unit": "ns/iter",
            "extra": "iterations: 37617\ncpu: 18654.218305553248 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/262144",
            "value": 18772.65663735557,
            "unit": "ns/iter",
            "extra": "iterations: 37756\ncpu: 18763.372232228103 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeCreation/1000000",
            "value": 18598.89623388983,
            "unit": "ns/iter",
            "extra": "iterations: 37864\ncpu: 18596.2146630044 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeLookup/1000",
            "value": 1885.9762361191802,
            "unit": "ns/iter",
            "extra": "iterations: 372456\ncpu: 1885.6955506153724 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeLookup/4096",
            "value": 1987.1957594845958,
            "unit": "ns/iter",
            "extra": "iterations: 349250\ncpu: 1986.91118396564 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeLookup/32768",
            "value": 2586.3027065056926,
            "unit": "ns/iter",
            "extra": "iterations: 272898\ncpu: 2585.9318793101984 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/NodeLookup/100000",
            "value": 2633.204103909188,
            "unit": "ns/iter",
            "extra": "iterations: 254684\ncpu: 2632.8131684754767 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeLookup/1000",
            "value": 1765.6620479157634,
            "unit": "ns/iter",
            "extra": "iterations: 391520\ncpu: 1765.514512668556 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeLookup/4096",
            "value": 1882.907438313303,
            "unit": "ns/iter",
            "extra": "iterations: 378688\ncpu: 1882.6718327488757 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeLookup/32768",
            "value": 2472.1083574739127,
            "unit": "ns/iter",
            "extra": "iterations: 285481\ncpu: 2471.6283535506673 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/EdgeLookup/100000",
            "value": 2684.638384649195,
            "unit": "ns/iter",
            "extra": "iterations: 261615\ncpu: 2684.2580127286174 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexCreation/1000",
            "value": 674.754734122668,
            "unit": "ns/iter",
            "extra": "iterations: 1024264\ncpu: 678.5088492755586 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexCreation/4096",
            "value": 681.1469077677159,
            "unit": "ns/iter",
            "extra": "iterations: 1019497\ncpu: 685.298476594351 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexCreation/32768",
            "value": 684.7630758055881,
            "unit": "ns/iter",
            "extra": "iterations: 1014544\ncpu: 689.4454168512599 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexCreation/100000",
            "value": 688.8515193812149,
            "unit": "ns/iter",
            "extra": "iterations: 1009438\ncpu: 692.1364481987412 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexLookup/1000",
            "value": 734.5216946567167,
            "unit": "ns/iter",
            "extra": "iterations: 949266\ncpu: 734.4388601298174 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexLookup/4096",
            "value": 2737.861455745244,
            "unit": "ns/iter",
            "extra": "iterations: 255182\ncpu: 2737.4395411901914 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexLookup/32768",
            "value": 21286.97947676402,
            "unit": "ns/iter",
            "extra": "iterations: 32987\ncpu: 21285.584199836536 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PropertyIndexLookup/100000",
            "value": 66006.324009103,
            "unit": "ns/iter",
            "extra": "iterations: 10546\ncpu: 65998.84543902978 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/AdjacencyLookup/1000",
            "value": 44793.137045764604,
            "unit": "ns/iter",
            "extra": "iterations: 16126\ncpu: 44786.9220513454 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/AdjacencyLookup/4096",
            "value": 45414.40699452162,
            "unit": "ns/iter",
            "extra": "iterations: 15698\ncpu: 45406.76430118372 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/AdjacencyLookup/32768",
            "value": 46442.025097985854,
            "unit": "ns/iter",
            "extra": "iterations: 14543\ncpu: 46434.83792890076 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/AdjacencyLookup/100000",
            "value": 45173.856151301756,
            "unit": "ns/iter",
            "extra": "iterations: 15704\ncpu: 45167.71746052072 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/SimpleQuery/1000",
            "value": 81.35964236905306,
            "unit": "ns/iter",
            "extra": "iterations: 8342343\ncpu: 81.35272225081044 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/SimpleQuery/4096",
            "value": 82.13846261066949,
            "unit": "ns/iter",
            "extra": "iterations: 8523059\ncpu: 82.13008651002107 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/SimpleQuery/32768",
            "value": 83.47975563200474,
            "unit": "ns/iter",
            "extra": "iterations: 8381294\ncpu: 83.469981723587 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/SimpleQuery/100000",
            "value": 84.05661222211083,
            "unit": "ns/iter",
            "extra": "iterations: 8339065\ncpu: 84.04889708858335 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PathFinding/1000",
            "value": 2206587.636042364,
            "unit": "ns/iter",
            "extra": "iterations: 283\ncpu: 2206207.9717314183 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PathFinding/4096",
            "value": 2244903.2832766343,
            "unit": "ns/iter",
            "extra": "iterations: 293\ncpu: 2244622.245733806 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/PathFinding/10000",
            "value": 2198911.2542954152,
            "unit": "ns/iter",
            "extra": "iterations: 291\ncpu: 2198522.4776632003 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/DocumentBacklinks/1000",
            "value": 333.2423850123915,
            "unit": "ns/iter",
            "extra": "iterations: 2117272\ncpu: 333.2094818237811 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/DocumentBacklinks/4096",
            "value": 331.31920800928043,
            "unit": "ns/iter",
            "extra": "iterations: 2116742\ncpu: 331.2865753124423 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/DocumentBacklinks/10000",
            "value": 331.53254354212675,
            "unit": "ns/iter",
            "extra": "iterations: 2029988\ncpu: 331.4992044287952 ns\nthreads: 1"
          },
          {
            "name": "BenchmarkFixture/WikiWorkload/iterations:100000",
            "value": 4307.4245200000405,
            "unit": "ns/iter",
            "extra": "iterations: 100000\ncpu: 4306.855129999861 ns\nthreads: 1"
          }
        ]
      }
    ]
  }
}