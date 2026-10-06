abstract class Vehicle {
    protected string brand;
    protected int year;

    Vehicle(string brand, int year) {
        this.brand = brand;
        this.year = year;
    }

    abstract fn fuelType() -> string;
    abstract fn maxSpeed() -> int;

    fn info() {
        println(this.brand + " (" + this.year + ") — " + this.fuelType() + ", max " + this.maxSpeed() + " km/h");
    }
}

class Car extends Vehicle {
    Car(string brand, int year) {
        super(brand, year);
    }

    fn fuelType() -> string {
        return "Gasoline ⛽";
    }

    fn maxSpeed() -> int {
        return 220;
    }
}

class ElectricCar extends Vehicle {
    ElectricCar(string brand, int year) {
        super(brand, year);
    }

    fn fuelType() -> string {
        return "Electric ⚡";
    }

    fn maxSpeed() -> int {
        return 250;
    }
}

class Bicycle extends Vehicle {
    Bicycle(string brand, int year) {
        super(brand, year);
    }

    fn fuelType() -> string {
        return "Human Power 🚴";
    }

    fn maxSpeed() -> int {
        return 45;
    }
}

fn main() {
    Car car = new Car("Toyota", 2024);
    ElectricCar tesla = new ElectricCar("Tesla", 2025);
    Bicycle bike = new Bicycle("Trek", 2023);

    println("=== Vehicle Fleet ===");
    println("");
    car.info();
    tesla.info();
    bike.info();
}
