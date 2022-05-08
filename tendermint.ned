//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

simple Node
{
    parameters:
        double waitTime @unit(s) = 1s;
        
    gates:
        input in[];
        output out[];
}

network Network
{
    parameters:
        int NodeNumber = 3;
        int roundNumber = 3;
        int epochNumber = 3;
        int f = 0;
        
    submodules:
        Nodes[NodeNumber]: Node;
        
    connections:
        for i = 0 .. NodeNumber - 1, for j = 0 .. NodeNumber - 1, if i != j
        {
            Nodes[i].out++ --> {delay = 0.005s;} --> Nodes[j].in++;
        }
}

