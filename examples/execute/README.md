

### Design of the execution api


### Design of the elasticity

**how the comm can be elastic**
We use the mona comm for in-staging execution; For the common rpc call, we use the margo addr.
The mona comm can work together with the mpi comm.

**how it relates with the mpi comm**
There are two groups, init group and the elasticity group, when the number of elasticity group is 0, we can not further decreasing the process, when the elasticity group become full, we can not further adding new processes.

The rank 0 process is the leader one which is used to sync the process view.

**when the mona comm is updated**
The mona comm is only updated when the `syncView` RPC is called by the client.


**how the mona comm is updated**
The mona comm is created based on the latest view of the mona addresses stored in the data staging processes.