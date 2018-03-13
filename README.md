# A Cryptographic Hash Function

If you have decided to create your own cryptographic hash function, the correct response is: Don't! 
There exist many well designed and more importantly, well tested cryptographic hash functions out there. Use them instead.
The chance of you being able to make one better is small, and the chance of you making something much worse is great. However, lets assume that you can't. Assume that you don't trust them for one reason or another. Many published functions have arbitrary constants. You may feel that the authors may (or may not) have tweaked the constants in some way to have a subtle weakness.


Constructing a new cryptographic hash is fairly difficult. It is a good idea to use something already existing. However, making a new one is possible, you just need to be aware of the many possible attacks an adversary might use. If you have an estimate of how nonlinear your round function is, then it is possible to work out how many rounds are needed to give a fairly good guarantee of security.

Here, we create a hash based on a folded high precision multiply. The primitive has high nonlinearity, so only nine rounds are required. We use a tree-based hashing structure to allow parallelization and to be resistant to many generic attacks. The resulting function is quite simple conceptually, which enables easy analysis.
