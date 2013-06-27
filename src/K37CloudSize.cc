// Authors: Spencer Behling and Benjamin Fenker 2013

// #include <fstream>

#include "K37CloudSize.hh"
#include "globals.hh"
#include "Randomize.hh"

K37CloudSize::K37CloudSize(G4ThreeVector cloud_center,
                           G4ThreeVector cloud_temperature,
                           G4ThreeVector cloud_size_start)
    : cloud_center_(cloud_center),
      initial_cloud_size_(cloud_size_start), cycleTime(2.9472*ms),
      expansion_before_polarized_(300*microsecond) {
  SetupSigma(cloud_temperature);
}

K37CloudSize::~K37CloudSize() {
}

void K37CloudSize::makeEvent() {
  // Position of the decaying particle at the instant the cloud starts expanding
  G4ThreeVector initial_position =
      G4ThreeVector(G4RandGauss::shoot(cloud_center_.x(),
                                       initial_cloud_size_.x()),
                    G4RandGauss::shoot(cloud_center_.y(),
                                       initial_cloud_size_.y()),
                    G4RandGauss::shoot(cloud_center_.z(),
                                       initial_cloud_size_.z()));

  // The trapping laser is off 200 us before the OP laser comes on and it takes
  // ~ 100 us for the atoms to polarize.  Therefore the atoms will have been
  // expanidng for at least 300 us (expansion_before_polarized_) before they
  // will be classified as "polarized" and counted.
  G4double decayTime = -10;
  while (decayTime < expansion_before_polarized_) {
    decayTime = cycleTime*G4UniformRand();
  }

  G4ThreeVector velocity =
      G4ThreeVector(G4RandGauss::shoot(0, velocity_sigma_.x()),
                    G4RandGauss::shoot(0, velocity_sigma_.y()),
                    G4RandGauss::shoot(0, velocity_sigma_.z()));

  SetFinalPosition(initial_position, velocity, decayTime);

  //  Outputs a text file with starting x, y, z, vx, vy, vz, time
  // std::ofstream ofs;
  // ofs.open("cloudData.dat", std::ofstream::out | std::ofstream::app);
  // ofs << initial_position.x()/mm << "\t" << initial_position.y()/mm << "\t"
  //     << initial_position.z()/mm << "\t"
  //     << velocity.x()/(mm/ns) << "\t" << velocity.y()/(mm/ns) << "\t"
  //     << velocity.z()/(mm/ns) << "\t" << decayTime/ns  << G4endl;
}

void K37CloudSize::SetupSigma(G4ThreeVector temperature) {
  velocity_sigma_.setX(CalcSigma(temperature.x()));
  velocity_sigma_.setY(CalcSigma(temperature.y()));
  velocity_sigma_.setZ(CalcSigma(temperature.z()));
}

G4double K37CloudSize::CalcSigma(G4double temperature) {
  // MB velocity distribution for each dimension is gaussian (normal) centered
  // at zero with standard deviation sigma = sqrt(kT/m)
  // G4 keeps masses in terms of energy so the c*c gives it actual velocity
  // units

  G4ParticleTable *particleTable = G4ParticleTable::GetParticleTable();
  // Mass of Ar37...
  G4double mass = particleTable -> GetIon(18, 37, 0) -> GetPDGMass();
  return sqrt(temperature * k_Boltzmann * c_squared / mass);
}

void K37CloudSize::SetFinalPosition(G4ThreeVector initial_position,
                                    G4ThreeVector velocity, G4double time) {
  final_position_ = initial_position;
  final_position_ += velocity*time;
  // G4cout << "Initial position: " << initial_position.x()/mm << " mm, "
  //        << initial_position.y()/mm << " mm, " << initial_position.z()/mm
  //        << " mm" << G4endl;
  // G4cout << "Velocity: " << velocity.x()/(mm/ns) << " mm/ns, "
  //        << velocity.y()/(mm/ns) << " mm/ns, " << velocity.z()/(mm/ns)
  //        << " mm/ns" << G4endl;
  // G4cout << "Time: " << time/ns << " ns " << G4endl;
  // G4cout << "Final position: " << final_position_.x()/mm << " mm, "
  //        << final_position_.y()/mm << " mm, " << final_position_.z()/mm
  //        << " mm" << G4endl;
}

void K37CloudSize::SetTemperature(G4ThreeVector temp) {
  if (temp.x() < 0 || temp.y() < 0 || temp.z() < 0) {
    G4cout << "ERROR! Cannot set negative temperature" << G4endl;
  } else {
    SetupSigma(temp);
  }
}

void K37CloudSize::SetTemperature(G4double temp) {
  SetTemperature(G4ThreeVector(temp, temp, temp));
}

void K37CloudSize::SetInitialCloudSize(G4ThreeVector size) {
  if (size.x() < 0 || size.y() < 0 || size.z() < 0) {
    G4cout << "ERROR! Cannot set negative cloud size" << G4endl;
  } else {
    initial_cloud_size_ = size;
  }
}

void K37CloudSize::SetInitialCloudSize(G4double size) {
  SetInitialCloudSize(G4ThreeVector(size, size, size));
}
