window.BENCHMARK_DATA = {
  "lastUpdate": 1752251945150,
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
      }
    ]
  }
}