
import example

l = [1,2,3]
# example.doubleEach(l)
print(example.doubled(l))
print(example.doubled(x for x in range(4)))

print(l)
example.doubleEach(l)
print(l)


ts = example.TestStruct(3)
print(repr(ts))


example.kwarg_parrot(voltage=480000,
                     state='a stiff',
                     action='voom',
                     type='Norwegian Blue')


# example.printEach(l)
example.foo('hey')

# print(l)
# 