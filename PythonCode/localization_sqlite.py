import sqlite3 as lite
import sys

def getTraingingData():
    try:
        con = lite.connect('measurements.db')

        with con:
            cur = con.cursor()
            cur.execute("SELECT * FROM training")

            rows = cur.fetchall()

    except lite.Error, e:

        if con:
            con.rollback()

        print "Error %s:" % e.args[0]
        sys.exit(1)

    finally:

        if con:
            con.close()

        return rows


def getColumnValues(dataset,column):
    i = 0
    newDataset = []
    while i < len(dataset):
        newDataset.append(dataset[i][column])
        i += 1
    return newDataset

def main():
    trainingSetRssi = []
    trainingSet = getTraingingData()
    for x in trainingSet:
        new = [x[1], x[2], x[3], x[4]]
        trainingSetRssi.append(new)

    # trainingSetPos = []
    # for x in trainingSet:
    #     new = [x[0], x[1]]
    #     trainingSetPos.append(new)
    trainingSetPos = getColumnValues(trainingSet, 0)

    from sklearn.neighbors import KNeighborsClassifier
    model = KNeighborsClassifier(n_neighbors=4)
    model.fit(trainingSetRssi, trainingSetPos)
    predicted = model.predict([[9, 37, 40, 85]])
    print(predicted)


main()
