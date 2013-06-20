#!/usr/bin/env python

# This script calculates the field generated by a z-polarised, x-propagating
# plane wave incident on a perfectly conducting object.

# Help Python find the bempp module
import sys,os
sys.path.append("..")

from bempp.lib import *
import numpy as np

# Parameters

wavelength = 0.5
k = 2 * np.pi / wavelength

# Boundary conditions

def evalDirichletIncData(point, normal):
    field = evalIncField(point)
    result = np.cross(field, normal, axis=0)
    return result

def evalIncField(point):
    x, y, z = point
    field = np.array([x * 0., y * 0., np.exp(1j * k * x)])
    return field

# Load mesh

grid = createGridFactory().importGmshGrid(
    "triangular", "../../examples/meshes/sphere-h-0.2.msh")

# Create quadrature strategy

accuracyOptions = createAccuracyOptions()
# Increase by 2 the order of quadrature rule used to approximate
# integrals of regular functions on pairs on elements
accuracyOptions.doubleRegular.setRelativeQuadratureOrder(2)
# Increase by 2 the order of quadrature rule used to approximate
# integrals of regular functions on single elements
accuracyOptions.singleRegular.setRelativeQuadratureOrder(2)
quadStrategy = createNumericalQuadratureStrategy(
    "float64", "complex128", accuracyOptions)

# Create assembly context

assemblyOptions = createAssemblyOptions()
context = createContext(quadStrategy, assemblyOptions)

# Initialize spaces

space = createRaviartThomas0VectorSpace(context, grid)

# Construct elementary operators

slpOp = createMaxwell3dSingleLayerBoundaryOperator(
    context, space, space, space, k, "SLP")
dlpOp = createMaxwell3dDoubleLayerBoundaryOperator(
    context, space, space, space, k, "DLP")
idOp = createMaxwell3dIdentityOperator(
    context, space, space, space, "Id")

# Form the left- and right-hand-side operators

lhsOp = slpOp
rhsOp = -(0.5 * idOp + dlpOp)

# Construct the grid function representing the (input) Dirichlet data

dirichletIncData = createGridFunction(
    context, space, space, evalDirichletIncData, True)
dirichletData = -dirichletIncData

# Construct the right-hand-side grid function

rhs = rhsOp * dirichletData

# Initialize the solver

invLhsOp = acaOperatorApproximateLuInverse(
    lhsOp.weakForm().asDiscreteAcaBoundaryOperator(), 1e-2)
prec = discreteOperatorToPreconditioner(invLhsOp)
solver = createDefaultIterativeSolver(lhsOp)
solver.initializeSolver(defaultGmresParameterList(1e-8), prec)

# Solve the equation

solution = solver.solve(rhs)
print solution.solverMessage()

# Extract the solution in the form of a grid function and export it
# in VTK format

neumannData = solution.gridFunction()
neumannData.exportToVtk("vertex_data", "neumann_data",
                        "calculated_neumann_data_vertex")


# Create potential operators

slPotOp = createMaxwell3dSingleLayerPotentialOperator(context, k)
dlPotOp = createMaxwell3dDoubleLayerPotentialOperator(context, k)

limits = [-2,2,-2,2,-2,2]
dims = [50,50,50]

# Use the Green's representation formula to evaluate the solution

from bempp import tools

evaluationPoints,scatteredField = tools.evaluatePotentialInBox([slPotOp,dlPotOp],[neumannData,dirichletData],
                                                               [-1,1],limits,dims)
incidentField = evalIncField(tools.mgridPoints2Array(evaluationPoints))

field = scatteredField + tools.array2Mgrid(incidentField,dims)
fieldMagnitude = np.sqrt(field[0].real ** 2 + field[0].imag ** 2 +
                         field[1].real ** 2 + field[1].imag ** 2 +
                         field[2].real ** 2 + field[2].imag ** 2)

# Plot data

from mayavi import mlab
from bempp.visualization2 import plotThreePlanes,plotGrid
plotThreePlanes(evaluationPoints,fieldMagnitude)
plotGrid(grid,representation='surface')
mlab.show()
