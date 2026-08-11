class Vector_3:
    x: float = 0
    y: float = 0
    z: float = 0

    def __init__(self, x: float, y: float, z: float):
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return f"{{{self.x},{self.y},{self.z}}}"


def vector3_normalize(v: Vector_3) -> Vector_3:
    length: float = pow(v.x * v.x + v.y * v.y + v.z * v.z, 0.5)
    if length == 0:
        return Vector_3(0, 0, 0)
    return Vector_3(v.x / len, v.y / len, v.z / len)


if __name__ == "__main__":
    v1 = Vector_3(1, 2, 3)
    print(vector3_normalize(v1))
