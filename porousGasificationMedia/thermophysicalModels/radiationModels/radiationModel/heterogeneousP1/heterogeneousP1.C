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

\*---------------------------------------------------------------------------*/

#include "heterogeneousP1.H"
#include "addToRunTimeSelectionTable.H"
#include "fvm.H"

#include "heterogeneousAbsorptionEmissionModel.H"
#include "scatterModel.H"
#include "mathematicalConstants.H"
#include "radiationConstants.H"

using namespace Foam::radiation;

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    namespace radiation
    {
        defineTypeNameAndDebug(heterogeneousP1, 0);

        addToRunTimeSelectionTable
        (
            heterogeneousRadiationModel,
            heterogeneousP1,
            porosity
        );
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::radiation::heterogeneousP1::heterogeneousP1(const volScalarField& T, const volScalarField& porosityF, const volScalarField& surfF, const volScalarField& Ts)
:
    heterogeneousRadiationModel(T, porosityF, surfF, Ts),
    G_
    (
        IOobject
        (
            "G",
            mesh_.time().timeName(),
            mesh_,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_
    ),
    Qr_
    (
        IOobject
        (
            "Qr",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("Qr", dimMass/pow3(dimTime), 0.0)
    ),
    a_
    (
        IOobject
        (
            "a",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        dimensionedScalar("a", dimless/dimLength, 0.0)
    ),
    as_
    (
        IOobject
        (
            "as",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        dimensionedScalar("as", dimless/dimLength, 0.0)
    ),
    borderAs_
    (
        IOobject
        (
            "borderAs",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        dimensionedScalar("borderAs", dimless/dimLength, 0.0)
    ),
    E_
    (
        IOobject
        (
            "E",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("E", dimMass/dimLength/pow3(dimTime), 0.0)
    ),
    borderL_
    (
        dimensionedScalar("borderL", dimLength, 0.0)
    ),
    porosityF_
    (
        porosityF
    ),
    surfFI_
    (
        surfF
    ),
    surfF_
    (
        IOobject
        (
            "surfF",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        scalar(0.0)
    ),
    whereIs_
    (
        IOobject
        (
            "whereIs",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        scalar(1.0)
    ),
    whereIsNot_
    (
        IOobject
        (
            "whereIsNot",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        scalar(0.0)
    ),
    solidSh_
    (
        IOobject
        (
            "radiationSolidSh",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("radiationSolidSh", dimMass/dimLength/pow3(dimTime), 0.0)
    )
{
    forAll(porosityF_,cellI)
    {
        if (porosityF_[cellI] > (1.0 - pow(10.0,-8.0)))
        {
	    whereIs_[cellI] = 0.0;
	    whereIsNot_[cellI] = 1.0;
        }
        else
        {
	    whereIs_[cellI] = 1.0;
	    whereIsNot_[cellI] = 0.0;
        }
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::radiation::heterogeneousP1::~heterogeneousP1()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::radiation::heterogeneousP1::read()
{
    if (heterogeneousRadiationModel::read())
    {
        // nothing to read
        return true;
    }
    else
    {
        return false;
    }
}

void Foam::radiation::heterogeneousP1::calculate()
{
    a_  = absorptionEmission_->a();
    as_ = absorptionEmission_->as();
    borderAs_ = absorptionEmission_->borderAs();
    E_  = absorptionEmission_->E();
    borderL_  = absorptionEmission_->borderL();
    const volScalarField sigmaEff(scatter_->sigmaEff());

    volScalarField surfV = surfF_*0;
    if ( ((mesh_.geometricD()).x() + (mesh_.geometricD()).y() + (mesh_.geometricD()).z() ) == 3)
    {
        surfV.internalField() = borderL_*pow(mesh_.V(),2./3.)*surfFI_.internalField();
        surfF_.internalField() = borderL_/pow(mesh_.V(),1./3.)*surfFI_.internalField();
    }
    else if ( ((mesh_.geometricD()).x() + (mesh_.geometricD()).y() + (mesh_.geometricD()).z() ) == 1)
    { 
        vector f(0,0,0);
        f.x() = -min((mesh_.geometricD()).x(),0);
        f.y() = -min((mesh_.geometricD()).y(),0);
        f.z() = -min((mesh_.geometricD()).z(),0);
        forAll(surfFI_,cellI)
        {
            if(surfFI_[cellI] > 0)
            {
                scalar cSurf = 0;
                forAll(mesh_.cells()[cellI],faceI)
                {
                    cSurf += mag(( f & (mesh_.Sf()[mesh_.cells()[cellI][faceI]]) ));
                }
                surfF_[cellI] = borderL_.value()/pow(cSurf/2.,1./2.);
            }
        }
    }
    else
    { 
        vector f(0,0,0);
        f.x() = max((mesh_.geometricD()).x(),0);
        f.y() = max((mesh_.geometricD()).y(),0);
        f.z() = max((mesh_.geometricD()).z(),0);
        forAll(surfFI_,cellI)
        {
            if(surfFI_[cellI] > 0)
            {
                scalar cSurf = 0;
                forAll(mesh_.cells()[cellI],faceI)
                {
                    cSurf += mag(( f & (mesh_.Sf()[mesh_.cells()[cellI][faceI]]) ));
                }
                surfF_[cellI] = borderL_.value()/(2*mesh_.V()[cellI]/cSurf);
            }
        }
    }
    scalar totSur = gSum(surfV);
    scalar totVol = 0;
    forAll(porosityF_,cellI)
    {
        if (porosityF_[cellI] > (1.0 - pow(10.0,-8.0)))
        {
            whereIs_[cellI] = 0.0;
   	        whereIsNot_[cellI] = 1.0;
        }
        else
        {
	        whereIs_[cellI] = 1.0;
	        whereIsNot_[cellI] = 0.0;
            totVol += mesh_.V()[cellI];
        }
    }
    reduce(totVol, sumOp<scalar>());

    Info << "Radiation active volume to porous media volume ratio: " << totSur/max(totVol,SMALL) << endl;

    const volScalarField gamma
    (
        IOobject
        (
            "gammaRad",
            G_.mesh().time().timeName(),
            G_.mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        1.0/(3.0*(a_ + as_*whereIs_ + borderAs_*surfF_ ) + sigmaEff)  // eqZx2uHGn010
    );

    // eqZx2uHGn013
    volScalarField solidRadiation = (as_*whereIs_ + borderAs_*surfF_ )*radiation::sigmaSB*pow4(Ts_);

    // eqZx2uHGn009
    solve
    (
        fvm::laplacian(gamma, G_)
      - fvm::Sp((a_ + as_*whereIs_ + borderAs_*surfF_ ), G_)
     ==
      - 4.0*(a_*radiation::sigmaSB*pow4(T_) + solidRadiation)
    );

    // eqZx2uHGn012
    forAll(G_,cellI)
    {
        if (surfF_[cellI]==0)
        {
            solidSh_[cellI] = (G_[cellI]*as_[cellI] - 4.0*solidRadiation[cellI])*whereIs_[cellI];
        }
        else
        {
            solidSh_[cellI] = (G_[cellI]*(as_[cellI] + surfF_[cellI]*borderAs_[cellI]) - 4.0*solidRadiation[cellI])*whereIs_[cellI];
        } 
    }

    forAll(mesh_.boundaryMesh(), patchI)
    {
        Qr_.boundaryField()[patchI] =
            -gamma.boundaryField()[patchI]*G_.boundaryField()[patchI].snGrad();
    }

}

Foam::tmp<Foam::volScalarField> Foam::radiation::heterogeneousP1::Rp() const
{
    return tmp<volScalarField>
    (
        new volScalarField
        (
            IOobject
            (
                "Rp",
                mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false
            ),
          4.0*absorptionEmission_->eCont()*radiation::sigmaSB
        )
    );
}

Foam::tmp<Foam::DimensionedField<Foam::scalar, Foam::volMesh> >
Foam::radiation::heterogeneousP1::Ru() const
{
    const DimensionedField<scalar, volMesh>& G =
        G_.dimensionedInternalField();
    const DimensionedField<scalar, volMesh> E =
        absorptionEmission_->ECont()().dimensionedInternalField();
    const DimensionedField<scalar, volMesh> a =
        absorptionEmission_->aCont()().dimensionedInternalField();

    return  (a*G - 4.0*E);  
}

Foam::tmp<Foam::volScalarField>
Foam::radiation::heterogeneousP1::solidSh() const
{
    return tmp<volScalarField>
    (
        new volScalarField
        (
            IOobject
            (
                "tSolidSh",
                mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false
            ),
            solidSh_
        )
    );
}


// ************************************************************************* //
