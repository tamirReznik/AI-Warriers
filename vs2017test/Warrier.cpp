#include "Warrier.h"

using namespace std;

Warrier::Warrier(int team, int enemyTeam, int row, int col, int number_of_rooms, int* rate, Room* gameRooms, Warrier** teammate, double security_map[MSZ][MSZ], void (*warrierGotHit)(int, int, double, int), bool* gameOver)
{
	isGameOver = gameOver;
	radarRate = (*rate);
	numOfWarriers = rate;
	rateCounter = 0;
	copySecurityMap(security_map);
	this->warrierGotHitCallBack = warrierGotHit;
	myTeammate = teammate;
	rooms = gameRooms;
	firstAid = FIRST_AID_INIT_AMOUNT;
	numOfRooms = number_of_rooms;
	this->myTeam = team;
	this->enemyTeam = enemyTeam;
	myNode = new Node(row, col, 0, nullptr);
	this->hp = 9;
	this->myAmmo = 0;
	hpThreshold = rand() % 10 + 10;// each warrier have uniqe threshold (The measure of courage)
}

void Warrier::calculateNextStep(int maze[MSZ][MSZ])
{
	calculateAttacks(maze);
	myRoom_targetRoom[0] = whichRoom(myNode, 0);
	//cout << "\nhp: " << hp << "\n";
	if (hp <= 0 || *isGameOver)
	{
		if ((*myTeammate)->GetHp() <= 0 && !(*isGameOver))
		{
			//Game Over
			*isGameOver = true;
		}
		maze[myNode->GetRow()][myNode->GetCol()] = SPACE;
		return;
	}

	if (path.empty())
		calcStepTargetNOTExist(maze);
	else
		calcStepTargetExist(maze);

	if (closeRadar(maze))
	{
		shootToKill(maze);

		if (currentTarget == enemyTeam && myRoom_targetRoom[0] != myRoom_targetRoom[1])
			path = stack<Node*>();

	}

	warrierStep(maze);
	checkMySurroundingForAmmoAndHP(maze);
	calculateAttacks(maze);

}

void Warrier::shootToKill(int maze[MSZ][MSZ])
{
	myRoom_targetRoom[0] = checkMyCurrentRoom();
	if (myAmmo > 0 && (*myBullet) == nullptr && (*myGrenade) == nullptr)
	{
		//cout << "\nshootTOkILL_PREPARE\n";
		int col = myNode->GetCol();
		int row = myNode->GetRow();
		double cx = (col - (MSZ / 2.0)) * (2.0 / MSZ);
		double cy = (row - (MSZ / 2.0)) * (2.0 / MSZ);

		if (myAmmo / NUM_BULLETS_IN_GRENADE > 0 && rand() % 100 < 45)
		{
			(*myGrenade) = new Grenade(cx, cy, enemyTeam, warrierGotHitCallBack);
			(*myGrenade)->Explode();
			myAmmo -= NUM_BULLETS_IN_GRENADE;
		}
		else
		{
			double hypotenuse = sqrt(pow(ObjectsInRoom[0]->GetRow() - row, 2) + pow(ObjectsInRoom[0]->GetCol() - col, 2));
			double dirY = (ObjectsInRoom[0]->GetRow() - row) / hypotenuse;
			double dirX = (ObjectsInRoom[0]->GetCol() - col) / hypotenuse;
			(*myBullet) = new Bullet(cx, cy, dirX, dirY, enemyTeam, warrierGotHitCallBack);

			(*myBullet)->Fire(true);
			myAmmo--;
		}

	}
	//cout << "\nmy ammo: " << myAmmo;
}

void Warrier::calcStepTargetNOTExist(int maze[MSZ][MSZ])
{
	bool targetFound = false;
	//warrier injured
	if (hp < hpThreshold)
	{
		//todo - target need to be the closest between teamate and medical station
		if ((targetFound = findClosestTarget(maze, MEDICINES)))
		{
			currentTarget = MEDICINES;
			findPath(maze);


		}
	}
	if (myAmmo < 1 && !targetFound)
	{
		//todo - target need to be ammo bunkker / warrier teamate
		if ((targetFound = findClosestTarget(maze, AMMO)))
		{
			currentTarget = AMMO;
			findPath(maze);
			//cout << "path not exist!\n";

		}
	}
	if (!targetFound)
	{

		if (findClosestTarget(maze, enemyTeam))
		{
			currentTarget = enemyTeam;
			findPath(maze);
		}
	}
}

void Warrier::calcStepTargetExist(int maze[MSZ][MSZ])
{
	bool targetFound = false;
	if (hp < 10)
	{
		if (currentTarget == MEDICINES)
			return;

		currentTarget = MEDICINES;
		//todo - target need to be the closest between teamate and medical station
		if (findClosestTarget(maze, MEDICINES))
		{
			findPath(maze);

			return;
		}
	}
	else if (myAmmo < 1)
	{
		if (currentTarget == AMMO)
			return;

		currentTarget = AMMO;
		if (findClosestTarget(maze, AMMO))
		{
			findPath(maze);

			return;
		}
	}

	if (currentTarget == myTeam)
		return;

	if ((*myTeammate)->GetHp() > 0 && (*myTeammate)->GetHp() < (*myTeammate)->hpThreshold && firstAid > 0 || ((*myTeammate)->GetAmmo() < 1 && myAmmo / 2 > 4))
	{
		currentTarget = myTeam;
		targetNode = (*myTeammate)->GetMyNode();
		findPath(maze);
		return;
	}

	if (currentTarget == enemyTeam)
		return;

	currentTarget = enemyTeam;

	if (findClosestTarget(maze, enemyTeam))
	{
		findPath(maze);
	}

}

void Warrier::findPath(int maze[MSZ][MSZ])
{
	findRoomsIndex(myNode, targetNode);

	//if warrier with target in same room
	if (myRoom_targetRoom[0] == myRoom_targetRoom[1] && myRoom_targetRoom[0] != -1)
	{
		//if enemy in same room - find safest spot to combat from 
		if (currentTarget == enemyTeam)
		{
			findSafeSpotInRoom(maze);
		}

		findPathInsideRoom(maze, targetNode);

	}
	else
	{
		findPathBetweenRooms2(myRoom_targetRoom[1], maze);
	}

}

void Warrier::calculateAttacks(int maze[MSZ][MSZ])
{
	if ((*myBullet) && (*myBullet)->IsMoving())
	{

		(*myBullet)->Move(maze);
		if ((*myBullet)->IsHit())
		{
			(*myBullet) = nullptr;
		}
	}
	if ((*myGrenade) && (*myGrenade)->IsExploded())
	{

		(*myGrenade)->Exploding(maze);
		if ((*myGrenade)->IsDone())
		{
			(*myGrenade) = nullptr;
		}
	}


}

// return true if found both rooms else false
bool Warrier::findRoomsIndex(Node* myNode, Node* targetNode)
{
	myRoom_targetRoom[0] = -1;
	myRoom_targetRoom[1] = -1;
	int flag = 0;
	for (int i = 0; i < numOfRooms; i++)
	{
		if ((myNode->GetCol() <= rooms[i].GetCenterCol() + rooms[i].GetWidth() / 2 && myNode->GetCol() >= rooms[i].GetCenterCol() - rooms[i].GetWidth() / 2)
			&& (myNode->GetRow() <= rooms[i].GetCenterRow() + rooms[i].GetHeight() / 2 && myNode->GetRow() >= rooms[i].GetCenterRow() - rooms[i].GetHeight() / 2))
		{
			myRoom_targetRoom[0] = i;
			flag++;
		}
		if ((targetNode->GetCol() <= (rooms[i].GetCenterCol() + rooms[i].GetWidth() / 2) && targetNode->GetCol() >= (rooms[i].GetCenterCol() - rooms[i].GetWidth() / 2))
			&& (targetNode->GetRow() <= rooms[i].GetCenterRow() + rooms[i].GetHeight() / 2 && targetNode->GetRow() >= rooms[i].GetCenterRow() - rooms[i].GetHeight() / 2))
		{
			myRoom_targetRoom[1] = i;
			flag++;
		}
		if (flag > 1)
		{
			return true;
		}
	}
	return false;
}

void Warrier::DrawMe(int maze[MSZ][MSZ])
{

	if (*myGrenade && myGrenade)
	{
		(*myGrenade)->DrawMe();

	}
	if (*myBullet && myBullet)
	{
		(*myBullet)->DrawMe();

	}

	if (hp > 0 && !(*isGameOver))
		maze[myNode->GetRow()][myNode->GetCol()] = this->myTeam;
	else
		maze[myNode->GetRow()][myNode->GetCol()] = SPACE;

}

//bfs; 
//if target found return true ; else false
bool Warrier::findClosestTarget(int maze[MSZ][MSZ], int target)
{
	copyMaze(maze);

	bool bfs_is_on = true;
	Node* current;
	int curr_row, curr_col;
	targetNode = nullptr;
	vector <Node*> BFSGrays;
	BFSGrays.push_back(myNode);

	while (bfs_is_on)
	{
		if (BFSGrays.empty()) // no more grays....no solution
		{

			cout << "ClosestTarget not found (BFS)\n";
			bfs_is_on = false;
			return false;
		}
		else
		{
			// take the first element, paint it black and add to grays its white neighbors
			current = BFSGrays.front();
			BFSGrays.erase(BFSGrays.begin()); // remove the front element
			curr_row = current->GetRow();
			curr_col = current->GetCol();
			// paint it black
			if (myMaze[curr_row][curr_col] == GRAY)
			{
				myMaze[curr_row][curr_col] = BLACK;
			}
			// look for white neighbors

			// up
			if (CheckNeighborClosestTarget(current, curr_row + 1, curr_col, &bfs_is_on, &BFSGrays, target)
				|| CheckNeighborClosestTarget(current, curr_row, curr_col + 1, &bfs_is_on, &BFSGrays, target)
				|| CheckNeighborClosestTarget(current, curr_row, curr_col - 1, &bfs_is_on, &BFSGrays, target)
				|| CheckNeighborClosestTarget(current, curr_row - 1, curr_col, &bfs_is_on, &BFSGrays, target))
				return true;
		}
	}//while
	return false;
}

//if target found return true ; else false
bool Warrier::CheckNeighborClosestTarget(Node* current, int row, int col, bool* bfs_is_on, std::vector <Node*>* BFSGrays, int target)
{
	int alternativeTarget = target;

	if (target == MEDICINES && (*myTeammate)->firstAid > 0 && (*myTeammate)->hp > (*myTeammate)->hpThreshold
		|| (target == AMMO && (*myTeammate)->GetAmmo() / 2 > 5))
	{
		alternativeTarget = myTeam;
	}

	if (myMaze[row][col] == target || myMaze[row][col] == alternativeTarget
		&& (myNode->GetRow() != row && myNode->GetCol() != col))
	{
		targetNode = new Node(row, col, current->GetG() + 1, current);
		myMaze[row][col] = GRAY;
		*bfs_is_on = false;
		//cout << "\nTarget: " << target << " row: " << row << " col: " << col << endl;
		return true;

	}
	else
	{
		if (myMaze[row][col] == SPACE
			|| myMaze[row][col] == myTeam
			|| myMaze[row][col] == enemyTeam) // paint it gray
		{
			myMaze[row][col] = GRAY;
			BFSGrays->push_back(new Node(row, col, current->GetG() + 1, current));
		}
		return false;
	}
}

//bfs; 
// returns true if enemy found in the room else false (still ammo and medicine could be found and added to objectsInRoom array);
bool Warrier::CheckNeighborRadar(Node* current, int row, int col, bool* bfs_is_on, std::vector <Node*>* BFSGrays)
{

	//if ((col <= rooms[myRoom_targetRoom[0]].GetCenterCol() + rooms[myRoom_targetRoom[0]].GetWidth() / 2 + 1 && col >= rooms[myRoom_targetRoom[0]].GetCenterCol() - rooms[myRoom_targetRoom[0]].GetWidth() / 2 - 1)
	//	&& (row <= rooms[myRoom_targetRoom[0]].GetCenterRow() + rooms[myRoom_targetRoom[0]].GetHeight() / 2 + 1 && row >= rooms[myRoom_targetRoom[0]].GetCenterRow() - rooms[myRoom_targetRoom[0]].GetHeight() / 2 - 1))
	if (whichRoom(new Node(row, col, 0, nullptr), -1) == myRoom_targetRoom[0])
	{
		if (myMaze[row][col] == AMMO)
		{
			ObjectsInRoom[2] = new Node(row, col, current->GetG() + 1, current);
		}

		if (myMaze[row][col] == MEDICINES)
		{
			ObjectsInRoom[1] = new Node(row, col, current->GetG() + 1, current);
		}

		if (myMaze[row][col] == enemyTeam)
		{
			ObjectsInRoom[0] = new Node(row, col, current->GetG() + 1, current);
			//targetNode = new Node(row, col, current->GetG() + 1, current);
			myMaze[row][col] = GRAY;
			*bfs_is_on = false;
			//cout << "\nenemy in the room: row: " << row << " col: " << col << "\n";
			return true;

		}

		if (myMaze[row][col] == SPACE || myMaze[row][col] == myTeam) // paint it gray
		{
			myMaze[row][col] = GRAY;
			BFSGrays->push_back(new Node(row, col, current->GetG() + 1, current));
		}

	}
	return false;
}

//bfs; 
// returns true if target found. else false;
bool Warrier::closeRadar(int maze[MSZ][MSZ])
{
	//return if tunnel
	if (myRoom_targetRoom[0] == -1)
	{
		return false;
	}
	copyMaze(maze);
	initObjectsInRoom();

	bool bfs_is_on = true;
	Node* current;
	int curr_row, curr_col;
	//targetNode = nullptr;
	vector <Node*> BFSGrays;
	BFSGrays.push_back(myNode);

	while (bfs_is_on)
	{
		if (BFSGrays.empty()) // no more grays....no solution
		{

			//cout << "warrier can't locate target (BFS)\n";
			bfs_is_on = false;
			return false;
		}
		else
		{
			// take the first element, paint it black and add to grays its white neighbors
			current = BFSGrays.front();
			BFSGrays.erase(BFSGrays.begin()); // remove the front element
			curr_row = current->GetRow();
			curr_col = current->GetCol();
			// paint it black
			if (myMaze[curr_row][curr_col] == GRAY)
			{
				myMaze[curr_row][curr_col] = BLACK;
			}
			// look for white neighbors

			// up
			if (CheckNeighborRadar(current, curr_row + 1, curr_col, &bfs_is_on, &BFSGrays)
				// right
				|| CheckNeighborRadar(current, curr_row, curr_col + 1, &bfs_is_on, &BFSGrays)
				// left
				|| CheckNeighborRadar(current, curr_row, curr_col - 1, &bfs_is_on, &BFSGrays)
				// down
				|| CheckNeighborRadar(current, curr_row - 1, curr_col, &bfs_is_on, &BFSGrays))
			{
				return true;
			}
			/*	// up
				if (CheckNeighborRadar(current, curr_row + 1, curr_col, &bfs_is_on, &BFSGrays))
					return true;
				// right
				if (CheckNeighborRadar(current, curr_row, curr_col + 1, &bfs_is_on, &BFSGrays))
					return true;
				// left
				if (CheckNeighborRadar(current, curr_row, curr_col - 1, &bfs_is_on, &BFSGrays))
					return true;
				// down
				if (CheckNeighborRadar(current, curr_row - 1, curr_col, &bfs_is_on, &BFSGrays))
					return true;
				*/
		}
	}//while
	return false;
}

void Warrier::initObjectsInRoom()
{
	for (int i = 0; i < 3; i++)
	{
		ObjectsInRoom[i] = nullptr;
	}
}

int Warrier::checkMyCurrentRoom()
{
	for (int i = 0; i < numOfRooms; i++)
	{
		if ((myNode->GetCol() <= rooms[i].GetCenterCol() + rooms[i].GetWidth() / 2 && myNode->GetCol() >= rooms[i].GetCenterCol() - rooms[i].GetWidth() / 2)
			&& (myNode->GetRow() <= rooms[i].GetCenterRow() + rooms[i].GetHeight() / 2 && myNode->GetRow() >= rooms[i].GetCenterRow() - rooms[i].GetHeight() / 2))
		{
			myRoom_targetRoom[0] = i;
			return i;
		}
	}
	return -1;
}

int Warrier::whichRoom(Node* node, int farFromEdge)
{
	for (int i = 0; i < numOfRooms; i++)
	{

		if (abs(node->GetCol() - rooms[i].GetCenterCol()) < rooms[i].GetWidth() / 2 - farFromEdge
			&& (abs(node->GetRow() - rooms[i].GetCenterRow()) < rooms[i].GetHeight() / 2 - farFromEdge))
		{
			return i;
		}
	}
	return -1;
}

//A-STAR
void Warrier::findPathInsideRoom(int maze[MSZ][MSZ], Node* target)
{

	copyMaze(maze);

	Node* current;
	priority_queue <Node*, vector<Node*>, CompareWarNodes> aStarGrays;
	int tRow = target->GetRow();
	int tCol = target->GetCol();
	aStarGrays.push(myNode);

	while (true)
	{
		//get cell with minimum(F) from priority queue
		current = aStarGrays.top();
		aStarGrays.pop();
		int currentRow = current->GetRow(), currentCol = current->GetCol();

		//checking each of the four neighbor cells
		if (CheckNeighborInsideRoom(current, new Node(currentRow + 1, currentCol, tRow, tCol, current->GetG() + 1, current, 100 * mySecurityMap[currentRow + 1][currentCol]), &aStarGrays)
			|| CheckNeighborInsideRoom(current, new Node(currentRow, currentCol + 1, tRow, tCol, current->GetG() + 1, current, 100 * mySecurityMap[currentRow][currentCol + 1]), &aStarGrays)
			|| CheckNeighborInsideRoom(current, new Node(currentRow - 1, currentCol, tRow, tCol, current->GetG() + 1, current, 100 * mySecurityMap[currentRow - 1][currentCol]), &aStarGrays)
			|| CheckNeighborInsideRoom(current, new Node(currentRow, currentCol - 1, tRow, tCol, current->GetG() + 1, current, 100 * mySecurityMap[currentRow][currentCol - 1]), &aStarGrays))
		{
			return;
		}

		//once the Cell is out of the queue need to paint it black his path from start not going to change
		if (myMaze[currentRow][currentCol] == GRAY)
			myMaze[currentRow][currentCol] = BLACK;

		//no more grays to check --> Can't find target
		if (aStarGrays.empty())
		{
			cout << "no solution - FindPath\n";
			return;
		}
	}
}

bool Warrier::CheckNeighborInsideRoom(Node* current, Node* neighbor, std::priority_queue<Node*, std::vector<Node*>, CompareWarNodes>* aStarGrays)
{

	Node tempNeighbor = *neighbor;

	//if (myMaze[neighbor->GetRow()][neighbor->GetCol()] == currentTarget)
	if (neighbor->DistanceFromMe(targetNode->GetRow(), targetNode->GetCol()) == 0)
	{

		//cout << "Solution Found! a-star\n";

		RestorePath(neighbor->GetParent());
		return true;
	}


	if (myMaze[neighbor->GetRow()][neighbor->GetCol()] == GRAY)
	{
		tempNeighbor.SetG(current->GetG() + 1);
		if (CompareWarNodes::compareCells(neighbor, &tempNeighbor))
		{
			neighbor->SetG(tempNeighbor.GetG());
			neighbor->SetParent(current);
		}
		return false;
	}

	if (myMaze[neighbor->GetRow()][neighbor->GetCol()] == SPACE)
	{
		neighbor->SetG(current->GetG() + 1);
		neighbor->SetParent(current);
		myMaze[neighbor->GetRow()][neighbor->GetCol()] = GRAY;
		aStarGrays->push(neighbor);
	}
	return false;
}

void Warrier::RestorePath(Node* current)
{
	path = stack<Node*>();
	while (current->GetRow() != this->myNode->GetRow() || current->GetCol() != this->myNode->GetCol())
	{
		path.push(current);

		current = current->GetParent();
	}
}

// run A* that finds the "best" path from warrier node to rooms[targetRoom]
void Warrier::findPathBetweenRooms2(int targetRoom, int maze[MSZ][MSZ])
{
	copyMaze(maze);
	int r, c, tr, tc;
	// create start Node
	r = myNode->GetRow();
	c = myNode->GetCol();
	tr = rooms[targetRoom].GetCenterRow();
	tc = rooms[targetRoom].GetCenterCol();
	Node pn = *(new Node(r, c, tr, tc, 0, nullptr, .0));

	Node* current;
	priority_queue <Node*, vector<Node*>, CompareWarNodes> aStarGrays;

	aStarGrays.push(myNode);

	while (true)
	{
		//get cell with minimum(F) from priority queue
		current = aStarGrays.top();
		aStarGrays.pop();
		int currentRow = current->GetRow(), currentCol = current->GetCol();


		//checking each of the four neighbor cells
		if (CheckNeighborBetweenRooms2(current, new Node(currentRow + 1, currentCol, tr, tc, current->GetG() + 1, current, 100 * mySecurityMap[currentRow + 1][currentCol]), &aStarGrays, targetRoom)
			|| CheckNeighborBetweenRooms2(current, new Node(currentRow, currentCol + 1, tr, tc, current->GetG() + 1, current, 100 * mySecurityMap[currentRow][currentCol + 1]), &aStarGrays, targetRoom)
			|| CheckNeighborBetweenRooms2(current, new Node(currentRow - 1, currentCol, tr, tc, current->GetG() + 1, current, 100 * mySecurityMap[currentRow - 1][currentCol]), &aStarGrays, targetRoom)
			|| CheckNeighborBetweenRooms2(current, new Node(currentRow, currentCol - 1, tr, tc, current->GetG() + 1, current, 100 * mySecurityMap[currentRow][currentCol - 1]), &aStarGrays, targetRoom))
		{
			return;
		}

		//once the Cell is out of the queue need to paint it black his path from start not going to change
		if (myMaze[currentRow][currentCol] == GRAY)
			myMaze[currentRow][currentCol] = BLACK;

		//no more grays to check --> Can't find target
		if (aStarGrays.empty())
		{
			cout << "no solution - FindPath\n";
			return;
		}
	}
}

bool Warrier::CheckNeighborBetweenRooms2(Node* current, Node* neighbor, std::priority_queue<Node*, std::vector<Node*>, CompareWarNodes>* aStarGrays, int targetRoom)
{


	if (whichRoom(neighbor, 1) == targetRoom) // this is target
	{
		RestorePath(current);
		return true;
	}
	Node tempNeighbor = *neighbor;

	if (myMaze[neighbor->GetRow()][neighbor->GetCol()] == GRAY)
	{
		tempNeighbor.SetG(current->GetG() + 1);
		if (CompareWarNodes::compareCells(neighbor, &tempNeighbor))
		{
			neighbor->SetG(tempNeighbor.GetG());
			neighbor->SetParent(current);
		}
		return false;
	}

	if (myMaze[neighbor->GetRow()][neighbor->GetCol()] == SPACE
		|| myMaze[neighbor->GetRow()][neighbor->GetCol()] == myTeam
		|| myMaze[neighbor->GetRow()][neighbor->GetCol()] == enemyTeam)
	{
		neighbor->SetG(current->GetG() + 1);
		neighbor->SetParent(current);
		myMaze[neighbor->GetRow()][neighbor->GetCol()] = GRAY;
		aStarGrays->push(neighbor);
	}
	return false;
}

double Warrier::distanceFromTeammate(int row, int col)
{

	return fabs(sqrt(pow(((*myTeammate)->GetMyNode()->GetRow() - row), 2) + pow(((*myTeammate)->GetMyNode()->GetCol() - col), 2)));

}

void Warrier::warrierStep(int maze[MSZ][MSZ])
{
	if (path.empty())
	{
		return;
	}
	Node* warrierNode = path.top();
	path.pop();
	//update main maze
	maze[myNode->GetRow()][myNode->GetCol()] = SPACE;
	maze[warrierNode->GetRow()][warrierNode->GetCol()] = myTeam;
	myNode = warrierNode;


}

//if ammo | medicine | teammate close to warrior then he can reload / increase hp
void Warrier::checkMySurroundingForAmmoAndHP(int maze[MSZ][MSZ])
{
	for (int i = myNode->GetRow() - 1; i <= myNode->GetRow() + 1; i++)
	{
		for (int j = myNode->GetCol() - 1; j <= myNode->GetCol() + 1; j++)
		{
			if (myAmmo >= MAX_AMMO && hp >= MAX_HP)
				return;

			if (i == myNode->GetRow() && j == myNode->GetCol())
				continue;

			if (maze[i][j] == AMMO)
			{
				myAmmo = min(myAmmo + MAX_AMMO / 4, MAX_AMMO);
				return;
			}
			if (maze[i][j] == MEDICINES)
			{
				hp = MAX_HP;
				return;
			}
			//share ammo between partners
			if (maze[i][j] == myTeam)
			{
				shareAmmoAndHp();
				return;
			}
		}
	}

}

void Warrier::shareAmmoAndHp()
{
	int partnerAmmo = (*myTeammate)->GetAmmo();
	int partnerFirstAid = (*myTeammate)->firstAid;

	if (partnerAmmo - partnerAmmo / 2 >= myAmmo + partnerAmmo / 2 && partnerAmmo - partnerAmmo / 2 > 0)
	{
		(*myTeammate)->SetAmmo(partnerAmmo - (partnerAmmo / 2));
		SetAmmo(myAmmo + partnerAmmo / 2);
	}
	if (hp < hpThreshold && partnerFirstAid > 0 && (*myTeammate)->GetHp() > 2 * (*myTeammate)->hpThreshold)
	{
		hp += FIRST_AID_HP;
		(*myTeammate)->SetFirstAid(partnerFirstAid - 1);
	}
}

void Warrier::copyMaze(int maze[MSZ][MSZ])
{
	int i, j;
	for (i = 0; i < MSZ; i++)
	{
		for (j = 0; j < MSZ; j++)
		{
			myMaze[i][j] = maze[i][j];
		}
	}
}

void Warrier::copySecurityMap(double securityMap[MSZ][MSZ])
{
	int i, j;
	for (i = 0; i < MSZ; i++)
		for (j = 0; j < MSZ; j++)
			mySecurityMap[i][j] = securityMap[i][j];
}

//BFS - find safest spot in the current room - for combat purposes
bool Warrier::findSafeSpotInRoom(int maze[MSZ][MSZ])
{
	copyMaze(maze);
	bool bfs_is_on = true;
	Node* current;
	int curr_row, curr_col;
	vector <Node*> BFSGrays;
	BFSGrays.push_back(myNode);
	double maxDistance = 2 * max(rooms[myRoom_targetRoom[0]].GetHeight(), rooms[myRoom_targetRoom[0]].GetWidth());
	while (bfs_is_on)
	{
		if (BFSGrays.empty()) // no more grays....no solution
		{
			cout << "safe target not found (BFS)\n";
			bfs_is_on = false;
			return false;
		}
		else
		{
			// take the first element, paint it black and add to grays its white neighbors
			current = BFSGrays.front();
			BFSGrays.erase(BFSGrays.begin()); // remove the front element
			curr_row = current->GetRow();
			curr_col = current->GetCol();

			// paint it black
			if (myMaze[curr_row][curr_col] == GRAY)
				myMaze[curr_row][curr_col] = BLACK;

			// look for white neighbors
			if (// up
				CheckNeighborSafeSpotInRoom(current, curr_row + 1, curr_col, &bfs_is_on, &BFSGrays)
				// right
				|| CheckNeighborSafeSpotInRoom(current, curr_row, curr_col + 1, &bfs_is_on, &BFSGrays)
				// left																				 
				|| CheckNeighborSafeSpotInRoom(current, curr_row, curr_col - 1, &bfs_is_on, &BFSGrays)
				// down																				 
				|| CheckNeighborSafeSpotInRoom(current, curr_row - 1, curr_col, &bfs_is_on, &BFSGrays))
				return true;
		}
	}//while

	return false;
}

//if target found return true ; else false
bool Warrier::CheckNeighborSafeSpotInRoom(Node* current, int row, int col, bool* bfs_is_on, std::vector <Node*>* BFSGrays)
{

	if (whichRoom(new Node(row, col, 0, nullptr), 0) != myRoom_targetRoom[0])
		return false;


	if (mySecurityMap[row][col] <= SAFE_PLACE_THRESHOLD
		&& targetNode->DistanceFromMe(row, col) > min(rooms[myRoom_targetRoom[0]].GetWidth() / 2, rooms[myRoom_targetRoom[0]].GetHeight() / 2))
	{
		targetNode = new Node(row, col, current->GetG() + 1, current);
		myMaze[row][col] = GRAY;
		*bfs_is_on = false;
		//cout << "\nfound safe place\n";
		return true;

	}
	else
	{
		if (myMaze[row][col] == SPACE || myMaze[row][col] == myTeam) // paint it gray
		{
			myMaze[row][col] = GRAY;
			BFSGrays->push_back(new Node(row, col, current->GetG() + 1, current));
		}
		return false;
	}
}

