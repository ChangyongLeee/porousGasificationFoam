/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::radiation::heterogeneousMeanTemp

Description

SourceFiles
    heterogeneousMeanTemp.C

\*---------------------------------------------------------------------------*/

#ifndef heterogeneousRadiationModelheterogeneousMeanTemp_H
#define heterogeneousRadiationModelheterogeneousMeanTemp_H

#include "heterogeneousRadiationModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace radiation
{

/*---------------------------------------------------------------------------*\
                           Class heterogeneousMeanTemp Declaration
\*---------------------------------------------------------------------------*/

class heterogeneousMeanTemp
:
    public heterogeneousRadiationModel
{
    // Private data

        //- Incident radiation / [W/m2]
        volScalarField G_;

        //- Total radiative heat flux [W/m2]
        volScalarField Qr_;

        //- Absorption coefficient
        volScalarField borderAs_;

        //- Emission contribution
        dimensionedScalar borderL_;

        //- porosity field
        const volScalarField& porosityF_;

        //- porosity field
        const volScalarField& surfFI_;

        //- porosity field
        volScalarField surfF_;

        //- magnitude of G_ field gradient
        volScalarField solidSh_;

    // Private Member Functions

        //- Disallow default bitwise copy construct
        heterogeneousMeanTemp(const heterogeneousMeanTemp&);

        //- Disallow default bitwise assignment
        void operator=(const heterogeneousMeanTemp&);


public:

    //- Runtime type information
    TypeName("heterogeneousMeanTemp");


    // Constructors

        //- Construct from components
        heterogeneousMeanTemp(const volScalarField& T, const volScalarField& porosityF, const volScalarField& surfF, const volScalarField& Ts);


    //- Destructor
    virtual ~heterogeneousMeanTemp();


    // Member functions

        // Edit

            //- Solve radiation equation(s)
            void calculate();

            //- Read radiation properties dictionary
            bool read();


        // Access

            //- Source term component (for power of T^4)
            virtual tmp<volScalarField> Rp() const;

            //- Source term component (constant)
            virtual tmp<DimensionedField<scalar, volMesh> > Ru() const;

            //- Source term component (constant)
            virtual tmp<volScalarField> solidSh() const;

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace radiation
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
