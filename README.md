# goober-eats-rust
Rewrite of an older school project in Rust.

## Build

To build this project (Why would you?), make sure you have rustup and cargo installed on your computer.

You can then clone this repository into wherever you please, and call `cargo run --release -- mapdata.txt deliveries.txt` to build and run the program.

## Rewriting things in Rust

This was a lot of fun to write in Rust, given that C++ is an esoteric and foolish language to write much of anything in.  
Rust handles pretty much all my use cases well, and luckily, creating an adjacency list doesn't require any recursive graph structures,
so we don't need any crazy Rc<RefCell<T>> patterns or unsafe blocks anywhere.  In fact, the project is 100% safe!  (Doesn't say much, but it's still cool.)

Pattern matching; method chaining; functional methods; *real* iterators; organization of "classes" (by composition of implementations and "traits"); *real* sum types; derive macros
for valuable functionality; unicode support and a great deal of thought put into String handling; explicit distinctions between cloning data, bitcopying data, and moving data around, passing around pointers (references); general speed and ease of use...
Rewriting this project in Rust took around two days, with very little debugging (most of which was done via println!(), as a memory debugger is unnecessary both for a project of this 
small scale, and in general, when writing compiling Rust code).

Meanwhile, when trying to add functionality I wanted in C++... I ran into a plethora of confusing obstacles, which eventually subsided without incidence.  I had a troubling time
defining a custom comparison function for std::pair<GeoCoord, Double> (because C++ doesn't have tuple primitives); which was very annoying and had me messing around with namespaces.
For some reason, I ended up defining a struct to hold my function in; it works, but it seems so absurd.  [This stackoverflow post](https://stackoverflow.com/questions/14016921/comparator-for-min-heap-in-c)
is the first result for when I google this sort of problem.  I am surprised that this is actually how you do it!  In Rust, comparison is a trait that can be custom implemented for your type.

```
// other and self are reversed, giving a reverse ordering.
impl Ord for Node<'_> {
    fn cmp(&self, other: &Self) -> Ordering {
        other.cost.cmp(&self.cost)
    }
}
```

It was incredibly simple and made a lot more sense than in C++.  You implement the ability to be compared/ordered for your type, Node.  You need to define the comparator function in cmp().
It returns an "Ordering", which is Rust's enum for order, composed simply of Equal, Greater, Less.  Usually, you call cmp on self first, then pass in the other; but by calling cmp on other
and passing in self, we reverse the ordering.

Meanwhile, my (admittedly rather inexperienced) implementation of a custom comparator in C++ looks like this:

```
struct newGreater {
    bool operator() (const pair<GeoCoord, double>& left, const pair<GeoCoord, double>& right) {
        return (left.second > right.second);
    }
};
```

Why does this function come packed in a class?  Why is the syntax so damn weird?  This "operator()" stuff... and opposed to Rust, this only returns a bool, because to compare two objects,
the program must sometimes make two comparisons.  In Rust, if this is the inner implementation, it is abstracted neatly away with the enum Ordering so it does not interfere with a programmer's line of thought.

## Performance benchmarks?

I downloaded hyperfine, which makes everything easy.  I feel sad for developers who were born earlier than me, to not have all these great tools ready for them.  I can say
nothing except that I was lucky enough to not have to work with the arcane stuff people wrote back then.  An easy benchmark, from powershell:

<code>
PS D:\rupo\goober_eats\target\release> hyperfine ".\goober_eats.exe ../../mapdata.txt ../../deliveries.txt"
Benchmark #1: .\goober_eats.exe ../../mapdata.txt ../../deliveries.txt                 0
  Time (mean ± σ):     810.2 ms ±  96.7 ms    [User: 2.4 ms, System: 7.2 ms]           1
  Range (min … max):   718.5 ms … 1003.8 ms    10 runs
</code>

<code
PS D:\cs\32\Project_4\Project4> hyperfine ".\goobereatscpp.exe mapdata.txt deliveries.txt"
Benchmark #1: .\goobereatscpp.exe mapdata.txt deliveries.txt                                                                                                                               
  Time (mean ± σ):     908.1 ms ±  84.2 ms    [User: 1.2 ms, System: 7.3 ms]                                                                                                               
  Range (min … max):   835.1 ms … 1094.5 ms    10 runs
</code>

Rust was compiled with --release; C++ was compiled with clang at 03.  I realize gcc is faster in general, but when I tried it with gcc, I was assaulted with a slew of errors likely related to some difference between something designed for linux and something designed on windows... gcc vs. msvc... etc.  I won't investigate that further for now.  It makes sense in a way to use clang, so that both rustc and clang are hosted on the LLVM.

Rust was actually at a disadvantage, technically; I had set it up to run 1000 iterations 100 times, making for 100,000 iterations in total; for C++ I set it to run plainly 50,000 times.  However, something curious took place, which illustrates to me the importance of testing your programs before you make bold claims about them.  My Rust implementation was
likely incorrect: my C++ implementation would optimize a 7.04 mile route down to 4.52 miles every single run, while my Rust implementation was considerably more random, and produced slightly less optimal routes (ranging from 4.5 to 5.2 miles, on average around 4.8).  I looked into the code and also saw that in Rust, I would reduce the temperature much more slowly in my simulated annealing (temp\*0.99), while in C++, I would reduce it very quickly (temp\*0.9).
Keeping this in mind, I might be arsed to correct these differences later.

Overall, I have no doubt that I implemented this "genetic algorithm" incorrectly.  It's okay; I rather dislike the method anyway.  I hope to design something better in the future.

EDIT: Made Rust slightly faster by removing one pointless Vec clone.

<code>
PS D:\rupo\goober_eats\target\release> hyperfine ".\goober_eats.exe ../../mapdata.txt ../../deliveries.txt"
Benchmark #1: .\goober_eats.exe ../../mapdata.txt ../../deliveries.txt                                                                                                     
  Time (mean ± σ):     763.9 ms ±  60.1 ms    [User: 0.0 ms, System: 6.4 ms]                                                                                                              
  Range (min … max):   702.3 ms … 894.1 ms    10 runs
</code>

Again, this is just me being bored; don't take this seriously.  I don't have any interest in improving the C++ version because working with C++ is horrible unless you're getting paid or rewarded in some way.  (In which case it becomes fun again!)

## Improvements?

Yes, obviously.  There is a massive problem with my optimizaiton method.  We were taught about "Simulated Annealing" as the cool thing to ask the professors about to use to optimize our route;
but there are problems with this method, as well as my implementation of it.  For one, it's non-deterministic; subsequent runs may produce different routes.

I have not implemented any sort of cache to write to a file and remember old tours.  This would involve the following changes:

<ol>
  <li>Parse deliveries into a HashSet of DeliveryRequests</li>
  <li>Implement serialization and deserialization for these sets of deliveries</li>
  <li>Write specific tour orders to a file (serialize them!), probably JSON, or if we're feeling smart, TOML or RON</li>
  <li>Sort this file into a dictionary of sorts, where a serialized hash of a set is associated with a serialized hash of a tour's order</li>
  <li>On run, check through file for deliveries set; if found, then recall tour and use that; skip optimization</li>
</ol>

This is what it would take to implement a tour optimization cache.  (Or so I think; I've never done it before.)

Well, to be frank, I would love instead to implement a different algorithm altogether.  I at a point in time considered using some sort of poor approximation algorithm, such as Christofide's
algorithm (bringing us at least into 3/2 of optimality); but I did not have the time to implement that.  However, if I'm going to be spending time on this algorithm,
I want to make it **really** good.

## Graph structures in Rust

So I've read up a lot on Stem-and-Cycle Reference Structures for solving the TSP with ejection chains.  There are plenty of papers on it.  I remember fondly asking Prof. Carey about optimization
for the TSP and he replied to my email with a link to arxiv.

Frankly, this whole S&C business is far too ambitious.  There are several parts to it.

<ul>
  <li>Implement a two-level tree (rather than a linked-list)</li>
  <li>Figure out what ejection chain methods are.</li>
  <li>Figure out how to actually write the S&C algorithms.</li>
</ul>

I actually implemented a full library for the 3rd problem, completely ignoring any academia-recommended data structures.  But it failed to build with one, single, cryptic error (thanks, MSVC).
I abandoned that work; it was unstable anyway.

My new plan?  To write the two-level tree structure.  I found an implementation of it on GitHub, the only one of its kind; I gave the one star it had some company and cloned it to my computer.

There are significant problems one runs into when writing recursive graph data-structures in Rust.  According to most people on StackOverflow, writing such structures is inherently unsafe.
In fact, it was annoying to find no completion of the "Ok Unsafe Deque" section in [this Rust linked list](https://rust-unofficial.github.io/too-many-lists/) tutorial, a very popular
book for learning Rust.

So it directs you to std::collections::LinkedList, which uses Option<NonNull<T>> and PhantomData, etc.  None of which really works for what I want.

Rc<RefCell<T>> is a bad idea, and generally only used to force - essentially, GC-style code into your program.  I couldn't either use a memory arena, because there would be situations
in which I would need to destroy nodes too; I couldn't just keep creating references, cloning them without a care in the world, and being fine with that.  (Though I think there is
still potential to this, and I plan to investigate it further.)

So I decided to abandon Rust entirely, and <strike>write in C</strike> write it in Rust, actually.  I can't believe I wrote that for a second.

I'm going to use an arena and charge through with it.  Anything is better than writing a custom growable vector type in C... well, depends.
